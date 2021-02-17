#include "common.h"
#include <ctype.h>

/*
 * SQLSetStmtAttr
 */

static char software_version[] = "$Id: attributes.c,v 1.6 2010-07-05 09:20:32 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int g_result = 0;
static unsigned int line_num;

static void
fatal(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
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
		fatal("Line %u: NULL int\n", line_num);
	l = strtol(s, &end, 0);
	if (end[0])
		fatal("Line %u: Invalid int\n", line_num);
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

static const char *
unlookup(int value, const struct lookup_int *table)
{
	if (!table)
		return "??";

	for (; table->name; ++table)
		if (table->value == value)
			return table->name;

	return "??";
}

static struct lookup_int concurrency[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_CONCUR_READ_ONLY),
	TYPE(SQL_CONCUR_LOCK),
	TYPE(SQL_CONCUR_ROWVER),
	TYPE(SQL_CONCUR_VALUES),
#undef TYPE
	{ NULL, 0 }
};

static struct lookup_int scrollable[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_NONSCROLLABLE),
	TYPE(SQL_SCROLLABLE),
#undef TYPE
	{ NULL, 0 }
};

static struct lookup_int sensitivity[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_UNSPECIFIED),
	TYPE(SQL_INSENSITIVE),
	TYPE(SQL_SENSITIVE),
#undef TYPE
	{ NULL, 0 }
};

static struct lookup_int cursor_type[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_CURSOR_FORWARD_ONLY),
	TYPE(SQL_CURSOR_STATIC),
	TYPE(SQL_CURSOR_KEYSET_DRIVEN),
	TYPE(SQL_CURSOR_DYNAMIC),
#undef TYPE
	{ NULL, 0 }
};

static struct lookup_int noscan[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_NOSCAN_OFF),
	TYPE(SQL_NOSCAN_ON),
#undef TYPE
	{ NULL, 0 }
};

static struct lookup_int retrieve_data[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_RD_ON),
	TYPE(SQL_RD_OFF),
#undef TYPE
	{ NULL, 0 }
};

static struct lookup_int simulate_cursor[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_SC_NON_UNIQUE),
	TYPE(SQL_SC_TRY_UNIQUE),
	TYPE(SQL_SC_UNIQUE),
#undef TYPE
	{ NULL, 0 }
};

static struct lookup_int use_bookmarks[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_UB_OFF),
	TYPE(SQL_UB_VARIABLE),
	TYPE(SQL_UB_FIXED),
#undef TYPE
	{ NULL, 0 }
};

typedef enum
{
	type_INTEGER,
	type_UINTEGER,
	type_SMALLINT,
	type_LEN,
	type_CHARP,
	type_DESC,
	type_VOIDP
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
	ATTR(SQL_ATTR_APP_PARAM_DESC, DESC),
	ATTR(SQL_ATTR_APP_ROW_DESC, DESC),
	ATTR(SQL_ATTR_ASYNC_ENABLE, UINTEGER),
	ATTR2(SQL_ATTR_CONCURRENCY, UINTEGER, concurrency),
	ATTR2(SQL_ATTR_CURSOR_SCROLLABLE, UINTEGER, scrollable),
	ATTR2(SQL_ATTR_CURSOR_SENSITIVITY, UINTEGER, sensitivity),
	ATTR2(SQL_ATTR_CURSOR_TYPE, UINTEGER, cursor_type),
	ATTR(SQL_ATTR_ENABLE_AUTO_IPD, UINTEGER),
	ATTR(SQL_ATTR_FETCH_BOOKMARK_PTR, VOIDP),
	ATTR(SQL_ATTR_IMP_PARAM_DESC, DESC),
	ATTR(SQL_ATTR_IMP_ROW_DESC, DESC),
	ATTR(SQL_ATTR_KEYSET_SIZE, UINTEGER),
	ATTR(SQL_ATTR_MAX_LENGTH, UINTEGER),
	ATTR(SQL_ATTR_MAX_ROWS, UINTEGER),
	ATTR(SQL_ATTR_METADATA_ID, UINTEGER),
	ATTR2(SQL_ATTR_NOSCAN, UINTEGER, noscan),
	ATTR(SQL_ATTR_PARAM_BIND_OFFSET_PTR, VOIDP),
	ATTR(SQL_ATTR_PARAM_BIND_OFFSET_PTR, VOIDP),
	ATTR(SQL_ATTR_PARAM_BIND_TYPE, UINTEGER),
	ATTR(SQL_ATTR_PARAM_OPERATION_PTR, VOIDP),
	ATTR(SQL_ATTR_PARAM_STATUS_PTR, VOIDP),
	ATTR(SQL_ATTR_PARAMS_PROCESSED_PTR, VOIDP),
	ATTR(SQL_ATTR_PARAMSET_SIZE, UINTEGER),
	ATTR(SQL_ATTR_QUERY_TIMEOUT, UINTEGER),
	ATTR2(SQL_ATTR_RETRIEVE_DATA, UINTEGER, retrieve_data),
	ATTR(SQL_ATTR_ROW_ARRAY_SIZE, UINTEGER),
	ATTR(SQL_ATTR_ROW_BIND_OFFSET_PTR, VOIDP),
	ATTR(SQL_ATTR_ROW_BIND_TYPE, UINTEGER),
	ATTR(SQL_ATTR_ROW_NUMBER, UINTEGER),
	ATTR(SQL_ATTR_ROW_OPERATION_PTR, VOIDP),
	ATTR(SQL_ATTR_ROW_STATUS_PTR, VOIDP),
	ATTR(SQL_ATTR_ROWS_FETCHED_PTR, VOIDP),
	ATTR2(SQL_ATTR_SIMULATE_CURSOR, UINTEGER, simulate_cursor),
	ATTR2(SQL_ATTR_USE_BOOKMARKS, UINTEGER, use_bookmarks),
#undef ATTR2
#undef ATTR
};

static const struct attribute *
lookup_attr(const char *name)
{
	unsigned int i;

	if (!name)
		fatal("Line %u: NULL attribute\n", line_num);
	for (i = 0; i < sizeof(attributes) / sizeof(attributes[0]); ++i)
		if (strcmp(attributes[i].name, name) == 0 || strcmp(attributes[i].name + 4, name) == 0)
			return &attributes[i];
	fatal("Line %u: attribute %s not found\n", line_num, name);
	return NULL;
}

#define SEP " \t\n"

#define ATTR_PARAMS const struct attribute *attr, int expected
typedef int (*get_attr_t) (ATTR_PARAMS);

static int
get_attr_stmt(ATTR_PARAMS)
{
	SQLINTEGER i, ind;
	SQLSMALLINT si;
	SQLLEN li;
	SQLRETURN ret;

	ret = SQL_ERROR;
	switch (attr->type) {
	case type_INTEGER:
	case type_UINTEGER:
		i = 0xdeadbeef;
		ret = SQLGetStmtAttr(odbc_stmt, attr->value, (SQLPOINTER) & i, sizeof(SQLINTEGER), &ind);
		break;
	case type_SMALLINT:
		si = 0xbeef;
		ret = SQLGetStmtAttr(odbc_stmt, attr->value, (SQLPOINTER) & si, sizeof(SQLSMALLINT), &ind);
		i = si;
		break;
	case type_LEN:
		li = 0xdeadbeef;
		ret = SQLGetStmtAttr(odbc_stmt, attr->value, (SQLPOINTER) & li, sizeof(SQLLEN), &ind);
		i = li;
		break;
	case type_VOIDP:
	case type_DESC:
	case type_CHARP:
		fatal("Line %u: CHAR* check still not supported\n", line_num);
		break;
	}
	if (!SQL_SUCCEEDED(ret))
		fatal("Line %u: failure not expected\n", line_num);
	return i;
}

#if 0
/* do not retry any attribute just return expected value so to make caller happy */
static int
get_attr_none(ATTR_PARAMS)
{
	return expected;
}
#endif

int
main(int argc, char *argv[])
{
#define TEST_FILE "attributes.in"
	const char *in_file = FREETDS_SRCDIR "/" TEST_FILE;
	FILE *f;
	char buf[256];
	SQLINTEGER i;
	SQLLEN len;
	get_attr_t get_attr_p = get_attr_stmt;

	odbc_connect();
	/* TODO find another way */
	odbc_check_cursor();
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

		if (strcmp(cmd, "odbc") == 0) {
			int odbc3 = get_int(strtok(NULL, SEP)) == 3 ? 1 : 0;

			if (odbc_use_version3 != odbc3) {
				odbc_use_version3 = odbc3;
				odbc_disconnect();
				odbc_connect();
				odbc_command("SET TEXTSIZE 4096");
				SQLBindCol(odbc_stmt, 1, SQL_C_SLONG, &i, sizeof(i), &len);
			}
			continue;
		}

		/* set attribute */
		if (strcmp(cmd, "set") == 0) {
			const struct attribute *attr = lookup_attr(strtok(NULL, SEP));
			char *value = strtok(NULL, SEP);
			SQLRETURN ret;

			if (!value)
				fatal("Line %u: value not defined\n", line_num);

			ret = SQL_ERROR;
			switch (attr->type) {
			case type_UINTEGER:
			case type_INTEGER:
				ret = SQLSetStmtAttr(odbc_stmt, attr->value, int2ptr(lookup(value, attr->lookup)),
						      sizeof(SQLINTEGER));
				break;
			case type_SMALLINT:
				ret = SQLSetStmtAttr(odbc_stmt, attr->value, int2ptr(lookup(value, attr->lookup)),
						      sizeof(SQLSMALLINT));
				break;
			case type_LEN:
				ret = SQLSetStmtAttr(odbc_stmt, attr->value, int2ptr(lookup(value, attr->lookup)),
						      sizeof(SQLLEN));
				break;
			case type_CHARP:
				ret = SQLSetStmtAttr(odbc_stmt, attr->value, (SQLPOINTER) value, SQL_NTS);
				break;
			case type_VOIDP:
			case type_DESC:
				fatal("Line %u: not implemented\n");
			}
			if (!SQL_SUCCEEDED(ret))
				fatal("Line %u: failure not expected setting statement attribute\n", line_num);
			get_attr_p = get_attr_stmt;
			continue;
		}

		/* test attribute */
		if (strcmp(cmd, "attr") == 0) {
			const struct attribute *attr = lookup_attr(strtok(NULL, SEP));
			char *value = strtok(NULL, SEP);
			int i, expected = lookup(value, attr->lookup);

			if (!value)
				fatal("Line %u: value not defined\n", line_num);

			i = get_attr_p(attr, expected);
			if (i != expected) {
				g_result = 1;
				fprintf(stderr, "Line %u: invalid %s got %d(%s) expected %s\n", line_num, attr->name, i, unlookup(i, attr->lookup), value);
			}
			continue;
		}

		if (strcmp(cmd, "reset") == 0) {
			odbc_reset_statement();
			continue;
		}

		fatal("Line %u: command '%s' not handled\n", line_num, cmd);
	}

	fclose(f);
	odbc_disconnect();

	printf("Done.\n");
	return g_result;
}
