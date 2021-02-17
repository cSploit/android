#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: ct_diagclient.c,v 1.9 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/* Testing: Client Messages */
int
main(int argc, char *argv[])
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	int verbose = 0;

	CS_RETCODE ret;
	CS_RETCODE results_ret;
	CS_INT result_type;
	CS_INT num_cols;

	CS_DATAFMT datafmt;
	CS_INT datalength[2];
	CS_SMALLINT ind[2];
	CS_INT count, row_count = 0;
	CS_INT cv;
	int i;
	CS_CHAR select[1024];

	CS_INT col1[2];
	CS_CHAR col2[2][5];
	CS_INT num_msgs, totmsgs;
	CS_CLIENTMSG client_message;
	int result = 1;

	fprintf(stdout, "%s: Retrieve data using array binding \n", __FILE__);
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

	totmsgs = 1;
	if (ct_diag(conn, CS_MSGLIMIT, CS_CLIENTMSG_TYPE, CS_UNUSED, &totmsgs) != CS_SUCCEED) {
		fprintf(stderr, "ct_diag(CS_MSGLIMIT) failed\n");
		return 1;
	}

	fprintf(stdout, "Maximum message limit is set to: %d\n", totmsgs);

	ret = run_command(cmd, "CREATE TABLE #ctlibarray (col1 int not null,  col2 char(4) not null, col3 datetime not null)");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlibarray values (1, 'AAAA', 'Jan  1 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlibarray values (2, 'BBBB', 'Jan  2 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	strcpy(select, "select col1, col2 from #ctlibarray order by col1 ");

	ret = ct_command(cmd, CS_LANG_CMD, select, CS_NULLTERM, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_command(%s) failed\n", select);
		return 1;
	}

	ret = ct_send(cmd);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_send() failed\n");
		return 1;
	}

	while ((results_ret = ct_results(cmd, &result_type)) == CS_SUCCEED) {
		switch ((int) result_type) {
		case CS_CMD_SUCCEED:
			break;
		case CS_CMD_DONE:
			break;
		case CS_CMD_FAIL:
			fprintf(stderr, "ct_results() result_type CS_CMD_FAIL.\n");
			return 1;
		case CS_ROW_RESULT:

			ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_res_info() failed");
				return 1;
			}
			if (num_cols != 2) {
				fprintf(stderr, "num_cols %d != 2", num_cols);
				return 1;
			}

			ret = ct_describe(cmd, 1, &datafmt);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_describe() failed");
				return 1;
			}
			datafmt.format = CS_FMT_UNUSED;
			if (datafmt.maxlength > 1024) {
				datafmt.maxlength = 1024;
			}

			datafmt.count = 2;

			ret = ct_bind(cmd, 1, &datafmt, &col1[0], datalength, ind);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}

			ret = ct_describe(cmd, 2, &datafmt);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_describe() failed");
				return 1;
			}

			datafmt.format = CS_FMT_NULLTERM;
			datafmt.maxlength = 5;
			datafmt.count = 4;

			ret = ct_bind(cmd, 2, &datafmt, &col2[0], datalength, ind);
			if (ret != CS_SUCCEED) {

				if (ct_diag(conn, CS_STATUS, CS_CLIENTMSG_TYPE, CS_UNUSED, &num_msgs) != CS_SUCCEED) {
					fprintf(stderr, "ct_diag(CS_STATUS) failed\n");
					return 1;
				}

				fprintf(stdout, "Number of client messages returned: %d\n", num_msgs);

				for (i = 0; i < num_msgs; i++) {

					if (ct_diag(conn, CS_GET, CS_CLIENTMSG_TYPE, i + 1, &client_message) != CS_SUCCEED) {
						fprintf(stderr, "cs_diag(CS_GET) failed\n");
						return 1;
					}

					clientmsg_cb(ctx, conn, &client_message);

				}

				if (ct_diag(conn, CS_CLEAR, CS_CLIENTMSG_TYPE, CS_UNUSED, NULL) != CS_SUCCEED) {
					fprintf(stderr, "cs_diag(CS_CLEAR) failed\n");
					return 1;
				}

				if (ct_diag(conn, CS_STATUS, CS_CLIENTMSG_TYPE, CS_UNUSED, &num_msgs) != CS_SUCCEED) {
					fprintf(stderr, "cs_diag(CS_STATUS) failed\n");
					return 1;
				}
				if (num_msgs != 0) {
					fprintf(stderr, "cs_diag(CS_CLEAR) failed there are still %d messages on queue\n",
						num_msgs);
					return 1;
				}
				
				/* we catch error, good */
				result = 0;
			} else {
				fprintf(stderr, "ct_bind() succeeded while it shouldn't\n");
				return 1;
			}
			datafmt.count = 2;
			ret = ct_bind(cmd, 2, &datafmt, &col2[0], datalength, ind);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}
			count = 0;
			while (((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) == CS_SUCCEED)
			       || (ret == CS_ROW_FAIL)) {
				row_count += count;
				if (ret == CS_ROW_FAIL) {
					fprintf(stderr, "ct_fetch() CS_ROW_FAIL on row %d.\n", row_count);
					return 1;
				} else {	/* ret == CS_SUCCEED */
					fprintf(stdout, "ct_fetch returned %d rows\n", count);
					for (cv = 0; cv < count; cv++)
						fprintf(stdout, "col1 = %d col2= '%s'\n", col1[cv], col2[cv]);
				}
				count = 0;
			}


			switch ((int) ret) {
			case CS_END_DATA:
				break;
			case CS_FAIL:
				fprintf(stderr, "ct_fetch() returned CS_FAIL.\n");
				return 1;
			default:
				fprintf(stderr, "ct_fetch() unexpected return.\n");
				return 1;
			}
			break;

		default:
			fprintf(stderr, "ct_results() unexpected result_type.\n");
			return 1;
		}
	}
	switch ((int) results_ret) {
	case CS_END_RESULTS:
		break;
	case CS_FAIL:
		fprintf(stderr, "ct_results() failed.\n");
		return 1;
		break;
	default:
		fprintf(stderr, "ct_results() unexpected return.\n");
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

	return result;
}
