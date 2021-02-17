#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <ctpublic.h>
#include <bkpublic.h>
#include "common.h"

static char software_version[] = "$Id: blk_out.c,v 1.6 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/* Testing: array binding of result set */
int
main(int argc, char *argv[])
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	CS_BLKDESC *blkdesc;
	int verbose = 0;

	CS_RETCODE ret;

	CS_DATAFMT datafmt;
	CS_INT count = 0;

	CS_INT  col1[2];
	CS_CHAR col2[2][5];
	CS_CHAR col3[2][32];
	CS_INT      lencol1[2];
	CS_SMALLINT indcol1[2];
	CS_INT      lencol2[2];
	CS_SMALLINT indcol2[2];
	CS_INT      lencol3[2];
	CS_SMALLINT indcol3[2];

	int i;


	fprintf(stdout, "%s: Retrieve data using array binding \n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	/* do not test error */
	ret = run_command(cmd, "IF OBJECT_ID('tempdb..#ctlibarray') IS NOT NULL DROP TABLE #ctlibarray");

	ret = run_command(cmd, "CREATE TABLE #ctlibarray (col1 int null,  col2 char(4) not null, col3 datetime not null)");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlibarray values (1, 'AAAA', 'Jan  1 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlibarray values (2, 'BBBB', 'Jan  2 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlibarray values (3, 'CCCC', 'Jan  3 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlibarray values (8, 'DDDD', 'Jan  4 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlibarray values (NULL, 'EEEE', 'Jan  5 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = blk_alloc(conn, BLK_VERSION_100, &blkdesc);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_alloc() failed\n");
		return 1;
	}

	ret = blk_init(blkdesc, CS_BLK_OUT, "#ctlibarray", CS_NULLTERM );

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_init() failed\n");
		return 1;
	}


	ret = blk_describe(blkdesc, 1, &datafmt);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_describe(1) failed");
		return 1;
	}

	datafmt.format = CS_FMT_UNUSED;
	if (datafmt.maxlength > 1024) {
		datafmt.maxlength = 1024;
	}

	datafmt.count = 2;

	ret = blk_bind(blkdesc, 1, &datafmt, &col1[0], &lencol1[0], &indcol1[0]);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_bind() failed\n");
		return 1;
	}


	ret = blk_describe(blkdesc, 2, &datafmt);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_describe(2) failed");
		return 1;
	}

	datafmt.format = CS_FMT_NULLTERM;
	datafmt.maxlength = 5;
	datafmt.count = 2;

	ret = blk_bind(blkdesc, 2, &datafmt, col2[0], &lencol2[0], &indcol2[0]);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "blk_bind() failed\n");
		return 1;
	}

	ret = blk_describe(blkdesc, 3, &datafmt);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_describe() failed");
		return 1;
	}

	datafmt.datatype = CS_CHAR_TYPE;
	datafmt.format = CS_FMT_NULLTERM;
	datafmt.maxlength = 32;
	datafmt.count = 2;

	ret = blk_bind(blkdesc, 3, &datafmt, col3[0], &lencol3[0], &indcol3[0]);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_bind() failed\n");
		return 1;
	}

	while((ret = blk_rowxfer_mult(blkdesc, &count)) == CS_SUCCEED) {
		for(i = 0; i < count; i++) {
			printf("retrieved %d (%d,%d) %s (%d,%d) %s (%d,%d)\n", 
				col1[i], lencol1[i], indcol1[i],
				col2[i], lencol2[i], indcol2[i],
				col3[i], lencol3[i], indcol3[i] );
		}
	}
	switch (ret) {
	case CS_END_DATA:
		for(i = 0; i < count; i++) {
			printf("retrieved %d (%d,%d) %s (%d,%d) %s (%d,%d)\n", 
				col1[i], lencol1[i], indcol1[i],
				col2[i], lencol2[i], indcol2[i],
				col3[i], lencol3[i], indcol3[i] );
		}
		break;
	case CS_FAIL:
	case CS_ROW_FAIL:
		fprintf(stderr, "blk_rowxfer_mult() failed\n");
		return 1;
	}

	blk_drop(blkdesc);

	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 1;
	}

	return 0;
}
