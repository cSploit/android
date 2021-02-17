#include "common.h"
#include <assert.h>

static char software_version[] = "$Id: type.c,v 1.9 2010-07-05 09:20:33 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

struct type
{
	SQLSMALLINT type;
	const char *name;
	unsigned flags;
};

#define FLAG_C  1
#define FLAG_SQL  2

#define TYPE_C(s) {s, #s, FLAG_C }
#define TYPE_SQL(s) {s, #s, FLAG_SQL }
#define TYPE_BOTH(s,s2) {s, #s, FLAG_SQL|FLAG_C }
/* 
 * Same define with test for constants
 *  #define TYPE_BOTH(s,s2) {s, #s, (FLAG_SQL|FLAG_C)+((1/!(s-s2))-1) }
 */
static const struct type types[] = {
	TYPE_BOTH(SQL_C_CHAR, SQL_CHAR),
	TYPE_BOTH(SQL_C_LONG, SQL_INTEGER),
	TYPE_BOTH(SQL_C_SHORT, SQL_SMALLINT),
	TYPE_BOTH(SQL_C_FLOAT, SQL_REAL),
	TYPE_BOTH(SQL_C_DOUBLE, SQL_DOUBLE),
	TYPE_BOTH(SQL_C_NUMERIC, SQL_NUMERIC),
	TYPE_C(SQL_C_DEFAULT),
	TYPE_C(SQL_C_DATE),
	TYPE_C(SQL_C_TIME),
/* MS ODBC do not support SQL_TIMESTAMP for IPD type while we support it */
/*	TYPE_C(SQL_C_TIMESTAMP), */
	TYPE_BOTH(SQL_C_TIMESTAMP, SQL_TIMESTAMP),
	TYPE_C(SQL_C_TYPE_DATE),
	TYPE_C(SQL_C_TYPE_TIME),
	TYPE_BOTH(SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP),
	TYPE_C(SQL_C_INTERVAL_YEAR),
	TYPE_C(SQL_C_INTERVAL_MONTH),
	TYPE_C(SQL_C_INTERVAL_DAY),
	TYPE_C(SQL_C_INTERVAL_HOUR),
	TYPE_C(SQL_C_INTERVAL_MINUTE),
	TYPE_C(SQL_C_INTERVAL_SECOND),
	TYPE_C(SQL_C_INTERVAL_YEAR_TO_MONTH),
	TYPE_C(SQL_C_INTERVAL_DAY_TO_HOUR),
	TYPE_C(SQL_C_INTERVAL_DAY_TO_MINUTE),
	TYPE_C(SQL_C_INTERVAL_DAY_TO_SECOND),
	TYPE_C(SQL_C_INTERVAL_HOUR_TO_MINUTE),
	TYPE_C(SQL_C_INTERVAL_HOUR_TO_SECOND),
	TYPE_C(SQL_C_INTERVAL_MINUTE_TO_SECOND),
	TYPE_BOTH(SQL_C_BINARY, SQL_BINARY),
	TYPE_BOTH(SQL_C_BIT, SQL_BIT),
	TYPE_C(SQL_C_SBIGINT),
	TYPE_C(SQL_C_UBIGINT),
	TYPE_BOTH(SQL_C_TINYINT, SQL_TINYINT),
	TYPE_C(SQL_C_SLONG),
	TYPE_C(SQL_C_SSHORT),
	TYPE_C(SQL_C_STINYINT),
	TYPE_C(SQL_C_ULONG),
	TYPE_C(SQL_C_USHORT),
	TYPE_C(SQL_C_UTINYINT),
	TYPE_BOTH(SQL_C_GUID, SQL_GUID),

	TYPE_SQL(SQL_BIGINT),
	TYPE_SQL(SQL_VARBINARY),
	TYPE_SQL(SQL_LONGVARBINARY),
	TYPE_SQL(SQL_VARCHAR),
	TYPE_SQL(SQL_LONGVARCHAR),
	TYPE_SQL(SQL_DECIMAL),
	TYPE_SQL(SQL_FLOAT),
	{0, NULL}
};

static const char *
get_type_name(SQLSMALLINT type)
{
	const struct type *p = types;

	for (; p->name; ++p)
		if (p->type == type)
			return p->name;
	return "(unknown)";
}

static int result = 0;

static void
check_msg(int check, const char *msg)
{
	if (check)
		return;
	fprintf(stderr, "Check failed: %s\n", msg);
	result = 1;
}

int
main(int argc, char **argv)
{
	const struct type *p;
	char buf[16];
	SQLINTEGER ind;
	SQLLEN lind;
	SQLHDESC desc;

	odbc_connect();

	/*
	 * test setting two time a descriptor
	 * success all user allocated are ARD or APD so type cheching can be done
	 * TODO freeing descriptor dissociate it from statements
	 */

	/* test C types */
	for (p = types; p->name; ++p) {
		if (SQL_SUCCEEDED
		    (SQLBindParameter(odbc_stmt, 1, SQL_PARAM_INPUT, p->type, SQL_VARCHAR, (SQLUINTEGER) (-1), 0, buf, 16, &lind))) {
			SQLSMALLINT concise_type, type, code;
			SQLHDESC desc;

			concise_type = type = code = 0;

			/* get APD */
			SQLGetStmtAttr(odbc_stmt, SQL_ATTR_APP_PARAM_DESC, &desc, sizeof(desc), &ind);

			SQLGetDescField(desc, 1, SQL_DESC_TYPE, &type, sizeof(SQLSMALLINT), &ind);
			SQLGetDescField(desc, 1, SQL_DESC_CONCISE_TYPE, &concise_type, sizeof(SQLSMALLINT), &ind);
			SQLGetDescField(desc, 1, SQL_DESC_DATETIME_INTERVAL_CODE, &code, sizeof(SQLSMALLINT), &ind);
			printf("Setted type %s -> [%d (%s), %d (%s), %d]\n",
			       p->name, (int) concise_type, get_type_name(concise_type), (int) type, get_type_name(type), code);
			check_msg(p->flags & FLAG_C, "Type not C successed to be set in APD");
		} else {
			SQLSMALLINT concise_type, type, code;
			SQLHDESC desc;

			concise_type = type = code = 0;

			fprintf(stderr, "Error setting type %d (%s)\n", (int) p->type, p->name);

			concise_type = p->type;
			SQLGetStmtAttr(odbc_stmt, SQL_ATTR_APP_PARAM_DESC, &desc, sizeof(desc), &ind);
			if (SQL_SUCCEEDED
			    (SQLSetDescField(desc, 1, SQL_DESC_CONCISE_TYPE, int2ptr(concise_type), sizeof(SQLSMALLINT))))
			{
				SQLGetDescField(desc, 1, SQL_DESC_TYPE, &type, sizeof(SQLSMALLINT), &ind);
				SQLGetDescField(desc, 1, SQL_DESC_CONCISE_TYPE, &concise_type, sizeof(SQLSMALLINT), &ind);
				SQLGetDescField(desc, 1, SQL_DESC_DATETIME_INTERVAL_CODE, &code, sizeof(SQLSMALLINT), &ind);
				printf("Setted type %s -> [%d (%s), %d (%s), %d]\n",
				       p->name,
				       (int) concise_type, get_type_name(concise_type), (int) type, get_type_name(type), code);
				check_msg(p->flags & FLAG_C, "Type not C successed to be set in APD");
			} else {
				check_msg(!(p->flags & FLAG_C), "Type C failed to be set in APD");
			}
		}
	}

	printf("\n\n");

	/* test SQL types */
	SQLGetStmtAttr(odbc_stmt, SQL_ATTR_IMP_PARAM_DESC, &desc, sizeof(desc), &ind);
	for (p = types; p->name; ++p) {
		SQLSMALLINT concise_type = p->type;

		if (SQL_SUCCEEDED
		    (SQLSetDescField(desc, 1, SQL_DESC_CONCISE_TYPE, int2ptr(concise_type), sizeof(SQLSMALLINT)))) {
			SQLSMALLINT concise_type, type, code;

			concise_type = type = code = 0;

			SQLGetDescField(desc, 1, SQL_DESC_TYPE, &type, sizeof(SQLSMALLINT), &ind);
			SQLGetDescField(desc, 1, SQL_DESC_CONCISE_TYPE, &concise_type, sizeof(SQLSMALLINT), &ind);
			SQLGetDescField(desc, 1, SQL_DESC_DATETIME_INTERVAL_CODE, &code, sizeof(SQLSMALLINT), &ind);
			printf("Setted type %s -> [%d (%s), %d (%s), %d]\n",
			       p->name, (int) concise_type, get_type_name(concise_type), (int) type, get_type_name(type), code);
			check_msg(p->flags & FLAG_SQL, "Type not SQL successed to be set in IPD");
		} else {
			fprintf(stderr, "Error setting type %d (%s)\n", (int) p->type, p->name);
			check_msg(!(p->flags & FLAG_SQL), "Type SQL failed to be set in IPD");
		}
	}

	odbc_disconnect();

	return result;
}
