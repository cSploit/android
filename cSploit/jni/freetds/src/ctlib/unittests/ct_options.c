#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: ct_options.c,v 1.5 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/* Testing: Set and get options with ct_options */
int
main(int argc, char *argv[])
{
	int verbose = 0;

	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	CS_RETCODE ret;

	CS_INT datefirst = 0;
	CS_INT dateformat = 0;
	CS_BOOL truefalse = 999;

	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}

	if (argc >= 5) {
		common_pwd.initialized = argc;
		strcpy(common_pwd.SERVER, argv[1]);
		strcpy(common_pwd.DATABASE, argv[2]);
		strcpy(common_pwd.USER, argv[3]);
		strcpy(common_pwd.PASSWORD, argv[4]);
	}

	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	fprintf(stdout, "%s: Set/Retrieve DATEFIRST\n", __FILE__);

	/* DATEFIRST */
	datefirst = CS_OPT_WEDNESDAY;
	if (ct_options(conn, CS_SET, CS_OPT_DATEFIRST, &datefirst, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_options() failed\n");
		return 1;
	}

	datefirst = 999;

	if (ct_options(conn, CS_GET, CS_OPT_DATEFIRST, &datefirst, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_options() failed\n");
		return 1;
	}
	if (datefirst != CS_OPT_WEDNESDAY) {
		fprintf(stderr, "ct_options(DATEFIRST) didn't work retrieved %d expected %d\n", datefirst, CS_OPT_WEDNESDAY);
		return 1;
	}

	fprintf(stdout, "%s: Set/Retrieve DATEFORMAT\n", __FILE__);

	/* DATEFORMAT */
	dateformat = CS_OPT_FMTMYD;
	if (ct_options(conn, CS_SET, CS_OPT_DATEFORMAT, &dateformat, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_options() failed\n");
		return 1;
	}

	dateformat = 999;

	if (ct_options(conn, CS_GET, CS_OPT_DATEFORMAT, &dateformat, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_options() failed\n");
		return 1;
	}
	if (dateformat != CS_OPT_FMTMYD) {
		fprintf(stderr, "ct_options(DATEFORMAT) didn't work retrieved %d expected %d\n", dateformat, CS_OPT_FMTMYD);
		return 1;
	}

	fprintf(stdout, "%s: Set/Retrieve ANSINULL\n", __FILE__);
	/* ANSI NULLS */
	truefalse = CS_TRUE;
	if (ct_options(conn, CS_SET, CS_OPT_ANSINULL, &truefalse, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_options() failed\n");
		return 1;
	}

	truefalse = 999;
	if (ct_options(conn, CS_GET, CS_OPT_ANSINULL, &truefalse, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_options() failed\n");
		return 1;
	}
	if (truefalse != CS_TRUE) {
		fprintf(stderr, "ct_options(ANSINULL) didn't work\n");
		return 1;
	}

	fprintf(stdout, "%s: Set/Retrieve CHAINXACTS\n", __FILE__);
	/* CHAINED XACT */
	truefalse = CS_TRUE;
	if (ct_options(conn, CS_SET, CS_OPT_CHAINXACTS, &truefalse, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_options() failed\n");
		return 1;
	}

	truefalse = 999;
	if (ct_options(conn, CS_GET, CS_OPT_CHAINXACTS, &truefalse, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_options() failed\n");
		return 1;
	}
	if (truefalse != CS_TRUE) {
		fprintf(stderr, "ct_options(CHAINXACTS) didn't work\n");
		return 1;
	}

	if (verbose) {
		fprintf(stdout, "Trying logout\n");
	}
	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 1;
	}

	return 0;
}
