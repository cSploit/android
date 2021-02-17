#include <config.h>

#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: ct_cursors.c,v 1.4 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int
main(int argc, char **argv)
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	CS_COMMAND *cmd2;
	CS_COMMAND *cmd3;
	CS_RETCODE ret;
	CS_RETCODE ret2;
	CS_RETCODE results_ret;
	CS_INT result_type;
	CS_INT count, count2, row_count = 0;
	CS_DATAFMT datafmt;
	CS_DATAFMT datafmt2;
	CS_SMALLINT ind;
	int verbose = 1;
	CS_CHAR *name = "c1";
	CS_CHAR *name2 = "c2";
	CS_CHAR col1[6];
	CS_CHAR col2[6];
	CS_INT datalength;
	CS_CHAR text[128];
	CS_INT num_cols, i;

	fprintf(stdout, "%s: use multiple cursors on the same connection\n", __FILE__);

	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	ret = ct_cmd_alloc(conn, &cmd2);
	if (ret != CS_SUCCEED) {
		if (verbose) {
			fprintf(stderr, "Command Alloc failed!\n");
		}
		return ret;
	}

	ret = ct_cmd_alloc(conn, &cmd3);
	if (ret != CS_SUCCEED) {
		if (verbose) {
			fprintf(stderr, "Command Alloc failed!\n");
		}
		return ret;
	}

	ret = run_command(cmd, "CREATE TABLE #test_table (col1 char(4))");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "INSERT #test_table (col1) VALUES ('AAA')");
	if (ret != CS_SUCCEED)
		return 1;
	ret = run_command(cmd, "INSERT #test_table (col1) VALUES ('BBB')");
	if (ret != CS_SUCCEED)
		return 1;
	ret = run_command(cmd, "INSERT #test_table (col1) VALUES ('CCC')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "CREATE TABLE #test_table2 (col1 char(4))");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "INSERT #test_table2 (col1) VALUES ('XXX')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "INSERT #test_table2 (col1) VALUES ('YYY')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "INSERT #test_table2 (col1) VALUES ('ZZZ')");


	if (ret != CS_SUCCEED)
		return 1;


	if (verbose) {
		fprintf(stdout, "opening first cursor on connection\n");
	}

	strcpy(text, "select col1 from #test_table order by col1");

	ret = ct_cursor(cmd, CS_CURSOR_DECLARE, name, CS_NULLTERM, text, CS_NULLTERM, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor declare failed\n");
		return 1;
	}

	ret = ct_cursor(cmd, CS_CURSOR_ROWS, name, CS_NULLTERM, NULL, CS_UNUSED, (CS_INT) 1);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor set cursor rows failed");
		return 1;
	}

	ret = ct_cursor(cmd, CS_CURSOR_OPEN, name, CS_NULLTERM, text, CS_NULLTERM, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor open failed\n");
		return 1;
	}

	ret = ct_send(cmd);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_send failed\n");
		return 1;
	}

	while ((results_ret = ct_results(cmd, &result_type)) == CS_SUCCEED) {
		switch ((int) result_type) {

		case CS_CMD_FAIL:
			fprintf(stderr, "ct_results failed\n");
			return 1;
		case CS_CMD_SUCCEED:
		case CS_CMD_DONE:
		case CS_STATUS_RESULT:
			break;

		case CS_CURSOR_RESULT:
			ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);

			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_res_info() failed");
				return 1;
			}

			if (num_cols != 1) {
				fprintf(stderr, "unexpected num of columns =  %d \n", num_cols);
				return 1;
			}

			for (i = 0; i < num_cols; i++) {

				/* here we can finally test for the return status column */
				ret = ct_describe(cmd, i + 1, &datafmt);

				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_describe() failed for column %d\n", i);
					return 1;
				}

				if (datafmt.status & CS_RETURN) {
					fprintf(stdout, "ct_describe() column %d \n", i);
				}

				datafmt.datatype = CS_CHAR_TYPE;
				datafmt.format = CS_FMT_NULLTERM;
				datafmt.maxlength = 6;
				datafmt.count = 1;
				datafmt.locale = NULL;
				ret = ct_bind(cmd, 1, &datafmt, col1, &datalength, &ind);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_bind() failed\n");
					return 1;
				}

			}
			break;

		case CS_COMPUTE_RESULT:
			fprintf(stderr, "ct_results() unexpected CS_COMPUTE_RESULT.\n");
			return 1;
		default:
			fprintf(stderr, "ct_results() unexpected result_type.\n");
			return 1;
		}
	}

	if (verbose) {
		fprintf(stdout, "opening second cursor on connection\n");
	}

	strcpy(text, "select col1 from #test_table2 order by col1");

	ret = ct_cursor(cmd2, CS_CURSOR_DECLARE, name2, CS_NULLTERM, text, CS_NULLTERM, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor declare failed\n");
		return 1;
	}

	ret = ct_cursor(cmd2, CS_CURSOR_ROWS, name2, CS_NULLTERM, NULL, CS_UNUSED, (CS_INT) 1);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor set cursor rows failed");
		return 1;
	}

	ret = ct_cursor(cmd2, CS_CURSOR_OPEN, name2, CS_NULLTERM, text, CS_NULLTERM, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor open failed\n");
		return 1;
	}

	ret = ct_send(cmd2);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_send failed\n");
		return 1;
	}

	while ((results_ret = ct_results(cmd2, &result_type)) == CS_SUCCEED) {
		switch ((int) result_type) {

		case CS_CMD_FAIL:
			fprintf(stderr, "ct_results failed\n");
			return 1;
		case CS_CMD_SUCCEED:
		case CS_CMD_DONE:
		case CS_STATUS_RESULT:
			break;

		case CS_CURSOR_RESULT:
			ret = ct_res_info(cmd2, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);

			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_res_info() failed");
				return 1;
			}

			if (num_cols != 1) {
				fprintf(stderr, "unexpected num of columns =  %d \n", num_cols);
				return 1;
			}

			for (i = 0; i < num_cols; i++) {

				/* here we can finally test for the return status column */
				ret = ct_describe(cmd2, i + 1, &datafmt2);

				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_describe() failed for column %d\n", i);
					return 1;
				}

				if (datafmt2.status & CS_RETURN) {
					fprintf(stdout, "ct_describe() column %d \n", i);
				}

				datafmt2.datatype = CS_CHAR_TYPE;
				datafmt2.format = CS_FMT_NULLTERM;
				datafmt2.maxlength = 6;
				datafmt2.count = 1;
				datafmt2.locale = NULL;
				ret = ct_bind(cmd2, 1, &datafmt2, col2, &datalength, &ind);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_bind() failed\n");
					return 1;
				}

			}
			break;

		case CS_COMPUTE_RESULT:
			fprintf(stderr, "ct_results() unexpected CS_COMPUTE_RESULT.\n");
			return 1;
		default:
			fprintf(stderr, "ct_results() unexpected result_type.\n");
			return 1;
		}
	}

	row_count = 0;

	while (((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) == CS_SUCCEED)
	       || (ret == CS_ROW_FAIL)) {

		ret2 = ct_fetch(cmd2, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count2);
		if (ret == CS_SUCCEED && ret2 == CS_SUCCEED) {
			printf("%s\t\t\t%s\n", col1, col2);
		}
	}

	switch ((int) ret) {
	case CS_END_DATA:
		break;
	case CS_ROW_FAIL:
		fprintf(stderr, "ct_fetch() CS_ROW_FAIL on row %d.\n", row_count);
		return 1;
	case CS_FAIL:
		fprintf(stderr, "ct_fetch() returned CS_FAIL.\n");
		return 1;
	default:
		fprintf(stderr, "ct_fetch() unexpected return.\n");
		return 1;
	}

	if (verbose) {
		fprintf(stdout, "closing first cursor on connection\n");
	}

	ret = ct_cursor(cmd, CS_CURSOR_CLOSE, name, CS_NULLTERM, NULL, CS_UNUSED, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor(close) failed\n");
		return ret;
	}

	if ((ret = ct_send(cmd)) != CS_SUCCEED) {
		fprintf(stderr, "BILL ct_send() failed\n");
		return ret;
	}

	while ((results_ret = ct_results(cmd, &result_type)) == CS_SUCCEED) {
		if (result_type == CS_CMD_FAIL) {
			fprintf(stderr, "ct_results(2) result_type CS_CMD_FAIL.\n");
			return 1;
		}
	}
	if (results_ret != CS_END_RESULTS) {
		fprintf(stderr, "ct_results() returned BAD.\n");
		return 1;
	}

	ret = ct_cursor(cmd, CS_CURSOR_DEALLOC, name, CS_NULLTERM, NULL, CS_UNUSED, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor(dealloc) failed\n");
		return ret;
	}

	if ((ret = ct_send(cmd)) != CS_SUCCEED) {
		fprintf(stderr, "ct_send() failed\n");
		return ret;
	}

	while ((results_ret = ct_results(cmd, &result_type)) == CS_SUCCEED) {
		if (result_type == CS_CMD_FAIL) {
			fprintf(stderr, "ct_results(3) result_type CS_CMD_FAIL.\n");
			return 1;
		}
	}
	if (results_ret != CS_END_RESULTS) {
		fprintf(stderr, "ct_results() returned BAD.\n");
		return 1;
	}

	if (verbose) {
		fprintf(stdout, "closing second cursor on connection\n");
	}

	ret = ct_cursor(cmd2, CS_CURSOR_CLOSE, name2, CS_NULLTERM, NULL, CS_UNUSED, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor(close) failed\n");
		return ret;
	}

	if ((ret = ct_send(cmd2)) != CS_SUCCEED) {
		fprintf(stderr, "BILL ct_send() failed\n");
		return ret;
	}

	while ((results_ret = ct_results(cmd2, &result_type)) == CS_SUCCEED) {
		if (result_type == CS_CMD_FAIL) {
			fprintf(stderr, "ct_results(2) result_type CS_CMD_FAIL.\n");
			return 1;
		}
	}
	if (results_ret != CS_END_RESULTS) {
		fprintf(stderr, "ct_results() returned BAD.\n");
		return 1;
	}

	ret = ct_cursor(cmd2, CS_CURSOR_DEALLOC, name2, CS_NULLTERM, NULL, CS_UNUSED, CS_UNUSED);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_cursor(dealloc) failed\n");
		return ret;
	}

	if ((ret = ct_send(cmd2)) != CS_SUCCEED) {
		fprintf(stderr, "ct_send() failed\n");
		return ret;
	}

	while ((results_ret = ct_results(cmd2, &result_type)) == CS_SUCCEED) {
		if (result_type == CS_CMD_FAIL) {
			fprintf(stderr, "ct_results(3) result_type CS_CMD_FAIL.\n");
			return 1;
		}
	}
	if (results_ret != CS_END_RESULTS) {
		fprintf(stderr, "ct_results() returned BAD.\n");
		return 1;
	}

	ct_cmd_drop(cmd2);
	ct_cmd_drop(cmd3);

	if (verbose) {
		fprintf(stdout, "Trying logout\n");
	}
	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 2;
	}

	if (verbose) {
		fprintf(stdout, "Test succeded\n");
	}
	return 0;
}
