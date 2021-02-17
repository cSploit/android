/*
 * Test from MATSUMOTO, Tadashi
 * Cfr "blk_init fails by the even number times execution" on ML, 2007-03-09
 * This mix bulk and cancel
 * $Id: blk_in2.c,v 1.2 2011-05-16 08:51:40 freddy77 Exp $
 */

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

static const char create_table_sql[] = "CREATE TABLE hogexxx (col varchar(100))";

static CS_RETCODE
hoge_blkin(CS_CONNECTION * con, CS_BLKDESC * blk, char *table, char *data)
{
	CS_DATAFMT meta = { "" };
	CS_INT length = 5;
	CS_INT row = 0;

	if (CS_SUCCEED != ct_cancel(con, NULL, CS_CANCEL_ALL))
		return CS_FAIL;
	if (CS_SUCCEED != blk_init(blk, CS_BLK_IN, table, CS_NULLTERM))
		return CS_FAIL;

	meta.count = 1;
	meta.datatype = CS_CHAR_TYPE;
	meta.format = CS_FMT_PADBLANK;
	meta.maxlength = 5;

	if (CS_SUCCEED != blk_bind(blk, (int) 1, &meta, data, &length, NULL))
		return CS_FAIL;
	if (CS_SUCCEED != blk_rowxfer(blk))
		return CS_FAIL;
	if (CS_SUCCEED != blk_done(blk, CS_BLK_ALL, &row))
		return CS_FAIL;

	return CS_SUCCEED;
}

int
main(int argc, char **argv)
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	CS_BLKDESC *blkdesc;
	int verbose = 0;
	int ret = 0;
	int i;
	char command[512];


	static char table_name[20] = "hogexxx";

	fprintf(stdout, "%s: Inserting data using bulk cancelling\n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	sprintf(command, "if exists (select 1 from sysobjects where type = 'U' and name = '%s') drop table %s",
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

	for (i = 0; i < 10; i++) {
		/* compute some data */
		memset(command, ' ', sizeof(command));
		memset(command, 'a' + i, (i * 37) % 11);

		ret = hoge_blkin(conn, blkdesc, table_name, command);
		if (ret != CS_SUCCEED)
			return 1;
	}

	blk_drop(blkdesc);

	/* TODO test correct insert */

	printf("done\n");

	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 1;
	}

	return 0;
}

