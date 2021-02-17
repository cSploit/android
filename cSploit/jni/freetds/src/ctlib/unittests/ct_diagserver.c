#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: ct_diagserver.c,v 1.5 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/* Testing: Server messages limit */
int
main(int argc, char *argv[])
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	int verbose = 0;
	CS_RETCODE ret;
	int i;
	CS_INT num_msgs, totMsgs;
	CS_SERVERMSG server_message;

	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	if (ct_diag(conn, CS_INIT, CS_UNUSED, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "ct_diag(CS_INIT) failed\n");
		return 1;
	}

	totMsgs = 4;

	if (ct_diag(conn, CS_MSGLIMIT, CS_SERVERMSG_TYPE, CS_UNUSED, &totMsgs) != CS_SUCCEED) {
		fprintf(stderr, "ct_diag(CS_STATUS) failed\n");
		return 1;
	}

	fprintf(stdout, "Maximum message limit is set to %d.\n", totMsgs);

	if (ct_diag(conn, CS_STATUS, CS_SERVERMSG_TYPE, CS_UNUSED, &num_msgs) != CS_SUCCEED) {
		fprintf(stderr, "ct_diag(CS_STATUS) failed\n");
		return 1;
	}

	fprintf(stdout, "Number of messages returned: %d\n", num_msgs);

	for (i = 0; i < num_msgs; i++) {

		if (ct_diag(conn, CS_GET, CS_SERVERMSG_TYPE, i + 1, &server_message) != CS_SUCCEED) {
			fprintf(stderr, "cs_diag(CS_GET) failed\n");
			return 1;
		}

		servermsg_cb(ctx, conn, &server_message);

	}

	if (ct_diag(conn, CS_CLEAR, CS_SERVERMSG_TYPE, CS_UNUSED, NULL) != CS_SUCCEED) {
		fprintf(stderr, "cs_diag(CS_CLEAR) failed\n");
		return 1;
	}

	if (ct_diag(conn, CS_STATUS, CS_SERVERMSG_TYPE, CS_UNUSED, &num_msgs) != CS_SUCCEED) {
		fprintf(stderr, "cs_diag(CS_STATUS) failed\n");
		return 1;
	}
	if (num_msgs != 0) {
		fprintf(stderr, "cs_diag(CS_CLEAR) failed there are still %d messages on queue\n", num_msgs);
		return 1;
	}

	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 1;
	}

	return 0;
}
