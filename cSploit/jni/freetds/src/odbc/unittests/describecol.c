#include "common.h"
#include <ctype.h>

/*
 * SQLDescribeCol test for precision
 * test what say SQLDescribeCol about precision using some type
 */

static char software_version[] = "$Id: describecol.c,v 1.21 2011-08-12 13:49:54 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int g_result = 0;
static unsigned int line_num;

#define SEP " \t\n"

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

static const char*
get_type(void)
{
	char *s = strtok(NULL, "");
	if (*s == '\"')
		return strtok(s+1, "\"");
	return strtok(s, SEP);
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

static const char*
unlookup(long int value, const struct lookup_int *table)
{
	static char buf[32];

	sprintf(buf, "%ld", value);
	if (!table)
		return buf;

	for (; table->name; ++table)
		if (table->value == value)
			return table->name;

	return buf;
}


static struct lookup_int sql_types[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_CHAR),
	TYPE(SQL_VARCHAR),
	TYPE(SQL_LONGVARCHAR),
	TYPE(SQL_WCHAR),
	TYPE(SQL_WVARCHAR),
	TYPE(SQL_WLONGVARCHAR),
	TYPE(SQL_DECIMAL),
	TYPE(SQL_NUMERIC),
	TYPE(SQL_SMALLINT),
	TYPE(SQL_INTEGER),
	TYPE(SQL_REAL),
	TYPE(SQL_FLOAT),
	TYPE(SQL_DOUBLE),
	TYPE(SQL_BIT),
	TYPE(SQL_TINYINT),
	TYPE(SQL_BIGINT),
	TYPE(SQL_BINARY),
	TYPE(SQL_VARBINARY),
	TYPE(SQL_LONGVARBINARY),
	TYPE(SQL_DATE),
	TYPE(SQL_TIME),
	TYPE(SQL_TIMESTAMP),
	TYPE(SQL_TYPE_DATE),
	TYPE(SQL_TYPE_TIME),
	TYPE(SQL_TYPE_TIMESTAMP),
	TYPE(SQL_DATETIME),
#undef TYPE
	{ NULL, 0 }
};

static struct lookup_int sql_bools[] = {
	{ "SQL_TRUE",  SQL_TRUE  },
	{ "SQL_FALSE", SQL_FALSE },
	{ NULL, 0 }
};

typedef enum
{
	type_INTEGER,
	type_SMALLINT,
	type_LEN,
	type_CHARP
} test_type_t;

struct attribute
{
	const char *name;
	int value;
	test_type_t type;
	const struct lookup_int *lookup;
};

static const struct attribute attributes[] = {
#define ATTR(s,t) { #s, s, type_##t, NULL }
#define ATTR2(s,t,l) { #s, s, type_##t, l }
	ATTR(SQL_COLUMN_LENGTH, INTEGER),
	ATTR(SQL_COLUMN_PRECISION, INTEGER),
	ATTR(SQL_COLUMN_SCALE, INTEGER),
	ATTR(SQL_DESC_LENGTH, LEN),
	ATTR(SQL_DESC_OCTET_LENGTH, LEN),
	ATTR(SQL_DESC_PRECISION, SMALLINT),
	ATTR(SQL_DESC_SCALE, SMALLINT),
	ATTR(SQL_DESC_DISPLAY_SIZE, INTEGER),
	ATTR(SQL_DESC_TYPE_NAME, CHARP),
	ATTR2(SQL_DESC_CONCISE_TYPE, SMALLINT, sql_types),
	ATTR2(SQL_DESC_TYPE, SMALLINT, sql_types),
	ATTR2(SQL_DESC_UNSIGNED, SMALLINT, sql_bools)
#undef ATTR2
#undef ATTR
};

static const struct attribute *
lookup_attr(const char *name)
{
	unsigned int i;

	if (!name)
		fatal(": NULL attribute\n");
	for (i = 0; i < sizeof(attributes) / sizeof(attributes[0]); ++i)
		if (strcmp(attributes[i].name, name) == 0 || strcmp(attributes[i].name + 4, name) == 0)
			return &attributes[i];
	fatal(": attribute %s not found\n", name);
	return NULL;
}

#define ATTR_PARAMS const struct attribute *attr, const char *expected_value
typedef void (*check_attr_t) (ATTR_PARAMS);

static void
check_attr_ird(ATTR_PARAMS)
{
	SQLLEN i;
	SQLRETURN ret;

	if (attr->type == type_CHARP) {
		char buf[256];
		SQLSMALLINT len;

		ret = SQLColAttribute(odbc_stmt, 1, attr->value, buf, sizeof(buf), &len, NULL);
		if (!SQL_SUCCEEDED(ret))
			fatal(": failure not expected\n");
		buf[sizeof(buf)-1] = 0;
		if (strcmp(C((SQLTCHAR*) buf), expected_value) != 0) {
			g_result = 1;
			fprintf(stderr, "Line %u: invalid %s got %s expected %s\n", line_num, attr->name, buf, expected_value);
		}
		return;
	}

	i = 0xdeadbeef;
	ret = SQLColAttribute(odbc_stmt, 1, attr->value, NULL, SQL_IS_INTEGER, NULL, &i);
	if (!SQL_SUCCEEDED(ret))
		fatal(": failure not expected\n");
	/* SQL_DESC_LENGTH is the same of SQLDescribeCol len */
	if (attr->value == SQL_DESC_LENGTH) {
		SQLSMALLINT si;
		SQLULEN li;
		CHKDescribeCol(1, NULL, 0, NULL, &si, &li, &si, &si, "S");
		if (i != li)
			fatal(": attr %s SQLDescribeCol len %ld != SQLColAttribute len %ld\n", attr->name, (long) li, (long) i);
	}
	if (i != lookup(expected_value, attr->lookup)) {
		g_result = 1;
		fprintf(stderr, "Line %u: invalid %s got %s expected %s\n", line_num, attr->name, unlookup(i, attr->lookup), expected_value);
	}
}

static void
check_attr_ard(ATTR_PARAMS)
{
	SQLINTEGER i, ind;
	SQLSMALLINT si;
	SQLLEN li;
	SQLRETURN ret;
	SQLHDESC desc = SQL_NULL_HDESC;
	char buf[256];

	/* get ARD */
	SQLGetStmtAttr(odbc_stmt, SQL_ATTR_APP_ROW_DESC, &desc, sizeof(desc), &ind);

	ret = SQL_ERROR;
	switch (attr->type) {
	case type_INTEGER:
		i = 0xdeadbeef;
		ret = SQLGetDescField(desc, 1, attr->value, (SQLPOINTER) & i, sizeof(SQLINTEGER), &ind);
		break;
	case type_SMALLINT:
		si = 0xbeef;
		ret = SQLGetDescField(desc, 1, attr->value, (SQLPOINTER) & si, sizeof(SQLSMALLINT), &ind);
		i = si;
		break;
	case type_LEN:
		li = 0xdeadbeef;
		ret = SQLGetDescField(desc, 1, attr->value, (SQLPOINTER) & li, sizeof(SQLLEN), &ind);
		i = li;
		break;
	case type_CHARP:
		ret = SQLGetDescField(desc, 1, attr->value, buf, sizeof(buf), &ind);
		if (!SQL_SUCCEEDED(ret))
			fatal(": failure not expected\n");
		if (strcmp(C((SQLTCHAR*) buf), expected_value) != 0) {
			g_result = 1;
			fprintf(stderr, "Line %u: invalid %s got %s expected %s\n", line_num, attr->name, buf, expected_value);
		}
		return;
	}
	if (!SQL_SUCCEEDED(ret))
		fatal(": failure not expected\n");
	if (i != lookup(expected_value, attr->lookup)) {
		g_result = 1;
		fprintf(stderr, "Line %u: invalid %s got %s expected %s\n", line_num, attr->name, unlookup(i, attr->lookup), expected_value);
	}
}

/* do not retry any attribute just return expected value so to make caller happy */
static void
check_attr_none(ATTR_PARAMS)
{
}

int
main(int argc, char *argv[])
{
#define TEST_FILE "describecol.in"
	const char *in_file = FREETDS_SRCDIR "/" TEST_FILE;
	FILE *f;
	char buf[256];
	SQLINTEGER i;
	SQLLEN len;
	check_attr_t check_attr_p = check_attr_none;

	odbc_connect();
	odbc_command("SET TEXTSIZE 4096");

	SQLBindCol(odbc_stmt, 1, SQL_C_SLONG, &i, sizeof(i), &len);

	f = fopen(in_file, "r");
	if (!f)
		f = fopen(TEST_FILE, "r");
	if (!f) {
		fprintf(stderr, "error opening test file\n");
		exit(1);
	}

	line_num = 0;
	while (fgets(buf, sizeof(buf), f)) {
		char *p = buf, *cmd;

		++line_num;

		while (isspace((unsigned char) *p))
			++p;
		cmd = strtok(p, SEP);

		/* skip comments */
		if (!cmd || cmd[0] == '#' || cmd[0] == 0 || cmd[0] == '\n')
			continue;

		ODBC_FREE();

		if (strcmp(cmd, "odbc") == 0) {
			int odbc3 = get_int(strtok(NULL, SEP)) == 3 ? 1 : 0;

			if (odbc_use_version3 != odbc3) {
				odbc_use_version3 = odbc3;
				odbc_disconnect();
				odbc_connect();
				odbc_command("SET TEXTSIZE 4096");
				SQLBindCol(odbc_stmt, 1, SQL_C_SLONG, &i, sizeof(i), &len);
			}
		}

		/* select type */
		if (strcmp(cmd, "select") == 0) {
			const char *type = get_type();
			const char *value = strtok(NULL, SEP);
			char sql[sizeof(buf) + 40];

			SQLMoreResults(odbc_stmt);
			odbc_reset_statement();

			sprintf(sql, "SELECT CONVERT(%s, %s) AS col", type, value);

			/* ignore error, we only need precision of known types */
			check_attr_p = check_attr_none;
			if (odbc_command_with_result(odbc_stmt, sql) != SQL_SUCCESS) {
				odbc_reset_statement();
				SQLBindCol(odbc_stmt, 1, SQL_C_SLONG, &i, sizeof(i), &len);
				continue;
			}

			CHKFetch("SI");
			SQLBindCol(odbc_stmt, 1, SQL_C_SLONG, &i, sizeof(i), &len);
			check_attr_p = check_attr_ird;
		}

		/* set attribute */
		if (strcmp(cmd, "set") == 0) {
			const struct attribute *attr = lookup_attr(strtok(NULL, SEP));
			char *value = strtok(NULL, SEP);
			SQLHDESC desc;
			SQLRETURN ret;
			SQLINTEGER ind;

			if (!value)
				fatal(": value not defined\n");

			/* get ARD */
			SQLGetStmtAttr(odbc_stmt, SQL_ATTR_APP_ROW_DESC, &desc, sizeof(desc), &ind);

			ret = SQL_ERROR;
			switch (attr->type) {
			case type_INTEGER:
				ret = SQLSetDescField(desc, 1, attr->value, int2ptr(lookup(value, attr->lookup)),
						      sizeof(SQLINTEGER));
				break;
			case type_SMALLINT:
				ret = SQLSetDescField(desc, 1, attr->value, int2ptr(lookup(value, attr->lookup)),
						      sizeof(SQLSMALLINT));
				break;
			case type_LEN:
				ret = SQLSetDescField(desc, 1, attr->value, int2ptr(lookup(value, attr->lookup)),
						      sizeof(SQLLEN));
				break;
			case type_CHARP:
				ret = SQLSetDescField(desc, 1, attr->value, (SQLPOINTER) value, SQL_NTS);
				break;
			}
			if (!SQL_SUCCEEDED(ret))
				fatal(": failure not expected setting ARD attribute\n");
			check_attr_p = check_attr_ard;
		}

		/* test attribute */
		if (strcmp(cmd, "attr") == 0) {
			const struct attribute *attr = lookup_attr(strtok(NULL, SEP));
			char *expected = strtok(NULL, SEP);

			if (!expected)
				fatal(": value not defined\n");

			check_attr_p(attr, expected);
		}
	}

	fclose(f);
	odbc_disconnect();

	printf("Done.\n");
	return g_result;
}
