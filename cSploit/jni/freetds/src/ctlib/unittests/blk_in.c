#include <config.h>

#include <stdio.h>
#include <assert.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */


#include <ctpublic.h>
#include <bkpublic.h>
#include "common.h"
#include "blk_in.h"

static CS_RETCODE
do_bind(CS_BLKDESC * blkdesc, int colnum, CS_INT host_format, CS_INT host_type, CS_INT host_maxlen, 
	void        *var_addr,
	CS_INT      *var_len_addr,
	CS_SMALLINT *var_ind_addr );

static char command[512];

/*
 * Static data for insertion
 */
int  not_null_bit = 1;
CS_INT      l_not_null_bit = 4;
CS_SMALLINT i_not_null_bit = 0;

char not_null_char[] = "a char";
CS_INT      l_not_null_char = 6;
CS_SMALLINT i_not_null_char = 0;

char not_null_varchar[] = "a varchar";
CS_INT      l_not_null_varchar = 9;
CS_SMALLINT i_not_null_varchar = 0;

char not_null_datetime[] 		= "Dec 17 2003  3:44PM";
CS_INT      l_not_null_datetime = 19; 
CS_SMALLINT i_not_null_datetime = 0;

char not_null_smalldatetime[] 	= "Dec 17 2003  3:44PM";
CS_INT      l_not_null_smalldatetime = 19;
CS_SMALLINT i_not_null_smalldatetime = 0;

char not_null_money[] = "12.34";
CS_INT      l_not_null_money = 5;
CS_SMALLINT i_not_null_money = 0;

char not_null_smallmoney[] = "12.34";
CS_INT      l_not_null_smallmoney = 5;
CS_SMALLINT i_not_null_smallmoney = 0;

char not_null_float[] = "12.34";
CS_INT      l_not_null_float = 5;
CS_SMALLINT i_not_null_float = 0;

char not_null_real[] = "12.34";
CS_INT      l_not_null_real = 5;
CS_SMALLINT i_not_null_real = 0;

char not_null_decimal[] = "12.34";
CS_INT      l_not_null_decimal = 5;
CS_SMALLINT i_not_null_decimal = 0;

char not_null_numeric[] = "12.34";
CS_INT      l_not_null_numeric = 5;
CS_SMALLINT i_not_null_numeric = 0;

int  not_null_int        = 1234;
CS_INT      l_not_null_int = 4;
CS_SMALLINT i_not_null_int = 0;

int  not_null_smallint   = 1234;
CS_INT      l_not_null_smallint = 4;
CS_SMALLINT i_not_null_smallint = 0;

int  not_null_tinyint    = 123;
CS_INT      l_not_null_tinyint = 4;
CS_SMALLINT i_not_null_tinyint = 0;

CS_INT      l_null_char = 0;
CS_SMALLINT i_null_char = -1;

CS_INT      l_null_varchar = 0;
CS_SMALLINT i_null_varchar = -1;

CS_INT      l_null_datetime = 0;
CS_SMALLINT i_null_datetime = -1;

CS_INT      l_null_smalldatetime = 0;
CS_SMALLINT i_null_smalldatetime = -1;

CS_INT      l_null_money = 0;
CS_SMALLINT i_null_money = -1;

CS_INT      l_null_smallmoney = 0;
CS_SMALLINT i_null_smallmoney = -1;

CS_INT      l_null_float = 0;
CS_SMALLINT i_null_float = -1;

CS_INT      l_null_real = 0;
CS_SMALLINT i_null_real = -1;

CS_INT      l_null_decimal = 0;
CS_SMALLINT i_null_decimal = -1;

CS_INT      l_null_numeric = 0;
CS_SMALLINT i_null_numeric = -1;

CS_INT      l_null_int = 0;
CS_SMALLINT i_null_int = -1;

CS_INT      l_null_smallint = 0;
CS_SMALLINT i_null_smallint = -1;

CS_INT      l_null_tinyint = 0;
CS_SMALLINT i_null_tinyint = -1;

static void
do_binds(CS_BLKDESC * blkdesc)
{
	enum { prefixlen = 0 };
	enum { termlen = 0 };
	enum NullValue { IsNull, IsNotNull };

	/* non nulls */

	do_bind(blkdesc, 1, CS_FMT_UNUSED,   CS_INT_TYPE,   4,  &not_null_bit, &l_not_null_bit, &i_not_null_bit);
	do_bind(blkdesc, 2, CS_FMT_NULLTERM, CS_CHAR_TYPE,  7,  not_null_char, &l_not_null_char, &i_not_null_char);
	do_bind(blkdesc, 3, CS_FMT_NULLTERM, CS_CHAR_TYPE,  10, not_null_varchar, &l_not_null_varchar, &i_not_null_varchar);
	do_bind(blkdesc, 4, CS_FMT_NULLTERM, CS_CHAR_TYPE,  20, not_null_datetime, &l_not_null_datetime, &i_not_null_datetime);
	do_bind(blkdesc, 5, CS_FMT_NULLTERM, CS_CHAR_TYPE,  20, not_null_smalldatetime, &l_not_null_smalldatetime, &i_not_null_smalldatetime);
	do_bind(blkdesc, 6, CS_FMT_NULLTERM, CS_CHAR_TYPE,  6,  not_null_money, &l_not_null_money, &i_not_null_money);
	do_bind(blkdesc, 7, CS_FMT_NULLTERM, CS_CHAR_TYPE,  6,  not_null_smallmoney, &l_not_null_smallmoney, &i_not_null_smallmoney);
	do_bind(blkdesc, 8, CS_FMT_NULLTERM, CS_CHAR_TYPE,  6,  not_null_float, &l_not_null_float, &i_not_null_float);
	do_bind(blkdesc, 9, CS_FMT_NULLTERM, CS_CHAR_TYPE,  6,  not_null_real, &l_not_null_real, &i_not_null_real);
	do_bind(blkdesc, 10, CS_FMT_NULLTERM, CS_CHAR_TYPE, 6,  not_null_decimal, &l_not_null_decimal, &i_not_null_decimal);
	do_bind(blkdesc, 11, CS_FMT_NULLTERM, CS_CHAR_TYPE, 6,  not_null_numeric, &l_not_null_numeric, &i_not_null_numeric);
	do_bind(blkdesc, 12, CS_FMT_UNUSED,   CS_INT_TYPE,  4,  &not_null_int, &l_not_null_int, &i_not_null_int);
	do_bind(blkdesc, 13, CS_FMT_UNUSED,   CS_INT_TYPE,  4,  &not_null_smallint, &l_not_null_smallint, &i_not_null_smallint);
	do_bind(blkdesc, 14, CS_FMT_UNUSED,   CS_INT_TYPE,  4,  &not_null_tinyint, &l_not_null_tinyint, &i_not_null_tinyint);

	/* nulls */

	do_bind(blkdesc, 15, CS_FMT_NULLTERM, CS_CHAR_TYPE, 7,  not_null_char, &l_null_char, &i_null_char);
	do_bind(blkdesc, 16, CS_FMT_NULLTERM, CS_CHAR_TYPE, 10, not_null_varchar, &l_null_varchar, &i_null_varchar);
	do_bind(blkdesc, 17, CS_FMT_NULLTERM, CS_CHAR_TYPE, 20, not_null_datetime, &l_null_datetime, &i_null_datetime);
	do_bind(blkdesc, 18, CS_FMT_NULLTERM, CS_CHAR_TYPE, 20, not_null_smalldatetime, &l_null_smalldatetime, &i_null_smalldatetime);
	do_bind(blkdesc, 19, CS_FMT_NULLTERM, CS_CHAR_TYPE, 6, not_null_money, &l_null_money, &i_null_money);
	do_bind(blkdesc, 20, CS_FMT_NULLTERM, CS_CHAR_TYPE, 6, not_null_smallmoney, &l_null_smallmoney, &i_null_smallmoney);
	do_bind(blkdesc, 21, CS_FMT_NULLTERM, CS_CHAR_TYPE, 6, not_null_float, &l_null_float, &i_null_float);
	do_bind(blkdesc, 22, CS_FMT_NULLTERM, CS_CHAR_TYPE, 6, not_null_real, &l_null_real, &i_null_real);
	do_bind(blkdesc, 23, CS_FMT_NULLTERM, CS_CHAR_TYPE, 6, not_null_decimal, &l_null_decimal, &i_null_decimal);
	do_bind(blkdesc, 24, CS_FMT_NULLTERM, CS_CHAR_TYPE, 6, not_null_numeric, &l_null_numeric, &i_null_numeric);
	do_bind(blkdesc, 25, CS_FMT_UNUSED,   CS_INT_TYPE,  4,  &not_null_int, &l_null_int, &i_null_int);
	do_bind(blkdesc, 26, CS_FMT_UNUSED,   CS_INT_TYPE,  4,  &not_null_smallint, &l_null_smallint, &i_null_smallint);
	do_bind(blkdesc, 27, CS_FMT_UNUSED,   CS_INT_TYPE,  4,  &not_null_tinyint, &l_null_tinyint, &i_null_tinyint);

}

static CS_RETCODE
do_bind(CS_BLKDESC * blkdesc, int colnum, CS_INT host_format, CS_INT host_type, CS_INT host_maxlen,
	void        *var_addr,
	CS_INT      *var_len_addr,
	CS_SMALLINT *var_ind_addr )
{
	CS_DATAFMT datafmt;
	CS_RETCODE ret;

	ret = blk_describe(blkdesc, colnum, &datafmt);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_describe(%d) failed", colnum);
		return ret;
	}

	datafmt.format = host_format;
	datafmt.datatype = host_type;
	datafmt.maxlength = host_maxlen;
	datafmt.count = 1;

	ret = blk_bind(blkdesc, colnum, &datafmt, var_addr, var_len_addr, var_ind_addr );
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_bind() failed\n");
		return ret;
	}
	return ret;
}


int
main(int argc, char **argv)
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	CS_BLKDESC *blkdesc;
	int verbose = 0;
	int count = 0;
	int ret;
	int i;

	const char *table_name = "all_types_bcp_unittest";

	fprintf(stdout, "%s: Retrieve data using array binding \n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	sprintf(command,"if exists (select 1 from sysobjects where type = 'U' and name = '%s') drop table %s", 
                    table_name, table_name);

	ret = run_command(cmd, command);
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, create_table_sql);
	if (ret != CS_SUCCEED)
		return 1;

	ret = blk_alloc(conn, BLK_VERSION_100, &blkdesc);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_alloc() failed\n");
		return 1;
	}

	ret = blk_init(blkdesc, CS_BLK_IN, (char *) table_name, CS_NULLTERM );

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_init() failed\n");
		return 1;
	}

	do_binds(blkdesc);

	fprintf(stdout, "Sending same row 10 times... \n");
	for (i=0; i<10; i++) {
		if((ret = blk_rowxfer(blkdesc)) != CS_SUCCEED) {
			fprintf(stderr, "blk_rowxfer() failed\n");
			return 1;
		}
	}

	ret = blk_done(blkdesc, CS_BLK_ALL, &count);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_done() failed\n");
		return 1;
	}

	blk_drop(blkdesc);

	printf("%d rows copied.\n", count);

	printf("done\n");

	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 1;
	}

	return 0;
}
