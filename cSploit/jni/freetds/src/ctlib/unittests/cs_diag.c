#include <config.h>

#include <stdio.h>
#include <cspublic.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: cs_diag.c,v 1.5 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/*
 * ct_send SQL |select name = @@servername|
 * ct_bind variable
 * ct_fetch and print results
 */
int
main(int argc, char **argv)
{
	int verbose = 1;
	CS_CONTEXT *ctx;
	CS_RETCODE ret;
	CS_DATAFMT srcfmt;
	CS_INT src = 32768;
	CS_DATAFMT dstfmt;
	CS_SMALLINT dst;
	CS_DATETIME dst_date;

	int i;

	CS_INT num_msgs;
	CS_CLIENTMSG client_message;

	fprintf(stdout, "%s: Testing context callbacks\n", __FILE__);

	srcfmt.datatype = CS_INT_TYPE;
	srcfmt.maxlength = sizeof(CS_INT);
	srcfmt.locale = NULL;

	dstfmt.datatype = CS_SMALLINT_TYPE;
	dstfmt.maxlength = sizeof(CS_SMALLINT);
	dstfmt.locale = NULL;

	if (verbose) {
		fprintf(stdout, "Trying clientmsg_cb with context\n");
	}
	if (cs_ctx_alloc(CS_VERSION_100, &ctx) != CS_SUCCEED) {
		fprintf(stderr, "cs_ctx_alloc() failed\n");
	}
	if (ct_init(ctx, CS_VERSION_100) != CS_SUCCEED) {
		fprintf(stderr, "ct_init() failed\n");
	}

	if (cs_diag(ctx, CS_INIT, CS_UNUSED, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "cs_diag(CS_INIT) failed\n");
		return 1;
	}

	if (cs_convert(ctx, &srcfmt, &src, &dstfmt, &dst, NULL) == CS_SUCCEED) {
		fprintf(stderr, "cs_convert() succeeded when failure was expected\n");
		return 1;
	}

	dstfmt.datatype = CS_DATETIME_TYPE;
	dstfmt.maxlength = sizeof(CS_DATETIME);
	dstfmt.locale = NULL;

	if (cs_convert(ctx, &srcfmt, &src, &dstfmt, &dst_date, NULL) == CS_SUCCEED) {
		fprintf(stderr, "cs_convert() succeeded when failure was expected\n");
		return 1;
	}

	if (cs_diag(ctx, CS_STATUS, CS_CLIENTMSG_TYPE, CS_UNUSED, &num_msgs) != CS_SUCCEED) {
		fprintf(stderr, "cs_diag(CS_STATUS) failed\n");
		return 1;
	}

	for (i = 0; i < num_msgs; i++ ) {

		if (cs_diag(ctx, CS_GET, CS_CLIENTMSG_TYPE, i + 1, &client_message) != CS_SUCCEED) {
			fprintf(stderr, "cs_diag(CS_GET) failed\n");
			return 1;
		}
	
	    cslibmsg_cb(NULL, &client_message);
	
	}

	if ((ret = cs_diag(ctx, CS_GET, CS_CLIENTMSG_TYPE, i + 1, &client_message)) != CS_NOMSG) {
		fprintf(stderr, "cs_diag(CS_GET) did not fail with CS_NOMSG\n");
		return 1;
	}

	if (cs_diag(ctx, CS_CLEAR, CS_CLIENTMSG_TYPE, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "cs_diag(CS_CLEAR) failed\n");
		return 1;
	}

	if (cs_diag(ctx, CS_STATUS, CS_CLIENTMSG_TYPE, CS_UNUSED, &num_msgs) != CS_SUCCEED) {
		fprintf(stderr, "cs_diag(CS_STATUS) failed\n");
		return 1;
	}
	if (num_msgs != 0) {
		fprintf(stderr, "cs_diag(CS_CLEAR) failed there are still %d messages on queue\n", num_msgs);
		return 1;
	}

	if (ct_exit(ctx, CS_UNUSED) != CS_SUCCEED) {
		fprintf(stderr, "ct_exit() failed\n");
	}
	if (cs_ctx_drop(ctx) != CS_SUCCEED) {
		fprintf(stderr, "cx_ctx_drop() failed\n");
	}

	return 0;
}
