#include <config.h>

#include <stdio.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: t0005.c,v 1.7 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char **argv)
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	CS_RETCODE ret;
	int verbose = 0;
	int i = 0;

		 /************** ADDED LINE **********************/

	fprintf(stdout, "%s: Testing login, logout\n", __FILE__);

	while (i++ < 100) {
		       /********** ADDED LINE *********************/

		if (verbose) {
			fprintf(stdout, "Trying login\n");
		}
		ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
		if (ret != CS_SUCCEED) {
			fprintf(stderr, "Login failed\n");
			return 1;
		}

		if (verbose) {
			fprintf(stdout, "Trying logout\n");
		}
		ret = try_ctlogout(ctx, conn, cmd, verbose);
		if (ret != CS_SUCCEED) {
			fprintf(stderr, "Logout failed\n");
			return 2;
		}


	}
     /**************************** ADDED LINE **********************/

	if (verbose) {
		fprintf(stdout, "Test succeeded\n");
	}
	return 0;
}
