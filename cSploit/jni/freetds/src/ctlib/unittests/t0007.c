#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: t0007.c,v 1.11 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/* Testing: Retrieve CS_TEXT_TYPE using ct_bind() */
int
main(int argc, char **argv)
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	int verbose = 0;

	CS_RETCODE ret;
	CS_RETCODE results_ret;
	CS_INT result_type;
	CS_INT num_cols;

	CS_DATAFMT datafmt[4];
	CS_INT datalength[4];
	CS_SMALLINT ind[4];
	CS_INT count, row_count = 0;

	CS_CHAR name[4][1024];

	name[0][0] = 0;
	name[1][0] = 0;
	name[2][0] = 0;
	name[3][0] = 0;

	fprintf(stdout, "%s: Retrieve CS_CHAR_TYPE using ct_bind()\n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	ret = ct_command(cmd, CS_LANG_CMD, "SELECT CONVERT(VARCHAR(7),'1234') AS test, CONVERT(VARCHAR(7),'') AS test2, CONVERT(VARCHAR(7),NULL) AS test3, CONVERT(NUMERIC(38,2), 123.45) as test4", CS_NULLTERM, CS_UNUSED);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_command() failed\n");
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
			if (num_cols != 4) {
				fprintf(stderr, "num_cols %d != 4", num_cols);
				return 1;
			}
			if (ct_describe(cmd, 1, &datafmt[0]) != CS_SUCCEED) {
				fprintf(stderr, "ct_describe() failed");
				return 1;
			}
			datafmt[0].format = CS_FMT_NULLTERM;
			++datafmt[0].maxlength;
			if (datafmt[0].maxlength > 1024) {
				datafmt[0].maxlength = 1024;
			}

			if (ct_describe(cmd, 2, &datafmt[1]) != CS_SUCCEED) {
				fprintf(stderr, "ct_describe() failed");
				return 1;
			}
			datafmt[1].format = CS_FMT_NULLTERM;
			++datafmt[1].maxlength;
			if (datafmt[1].maxlength > 1024) {
				datafmt[1].maxlength = 1024;
			}

			if (ct_describe(cmd, 3, &datafmt[2]) != CS_SUCCEED) {
				fprintf(stderr, "ct_describe() failed\n");
				return 1;
			}
			datafmt[2].format = CS_FMT_NULLTERM;
			++datafmt[2].maxlength;
			if (datafmt[2].maxlength > 1024) {
				datafmt[2].maxlength = 1024;
			}

			if (ct_describe(cmd, 4, &datafmt[3]) != CS_SUCCEED) {
				fprintf(stderr, "ct_describe() failed\n");
				return 1;
			}
			datafmt[3].format = CS_FMT_NULLTERM;
			if (datafmt[3].maxlength != sizeof(CS_NUMERIC)) {
				fprintf(stderr, "wrong maxlength for numeric\n");
				return 1;
			}
			++datafmt[3].maxlength;
			if (datafmt[3].maxlength > 1024) {
				datafmt[3].maxlength = 1024;
			}

			if (ct_bind(cmd, 1, &datafmt[0], name[0], &datalength[0], &ind[0]) != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}
			if (ct_bind(cmd, 2, &datafmt[1], name[1], &datalength[1], &ind[1]) != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}
			if (ct_bind(cmd, 3, &datafmt[2], name[2], &datalength[2], &ind[2]) != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}
			if (ct_bind(cmd, 4, &datafmt[3], name[3], &datalength[3], &ind[3]) != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}

			while (((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) == CS_SUCCEED)
			       || (ret == CS_ROW_FAIL)) {
				row_count += count;
				if (ret == CS_ROW_FAIL) {
					fprintf(stderr, "ct_fetch() CS_ROW_FAIL on row %d.\n", row_count);
					return 1;
				} else {	/* ret == CS_SUCCEED */
					if (verbose) {
						fprintf(stdout, "name = '%s'\n", name[0]);
					}
					if (ind[0] != 0) {
						fprintf(stderr, "Returned NULL\n");
						return 1;
					}
					if (strcmp(name[0], "1234")) {
						fprintf(stderr, "Bad return:\n'%s'\n! =\n'%s'\n", name[0], "1234");
						return 1;
					}
					if (datalength[0] != strlen(name[0]) + 1) {
						fprintf(stderr, "Bad length:\n'%ld'\n! =\n'%d'\n", (long) strlen(name[0]) + 1, datalength[0]);
						return 1;
					}
					if (ind[1] != 0) {
						fprintf(stderr, "Returned NULL\n");
						return 1;
					}
					/* empty are retunerd as a single space in TDS4.x and TDS5 */
					if (strcmp(name[1], "") && strcmp(name[1], " ")) {
						fprintf(stderr, "Bad return:\n'%s'\n! =\n'%s'\n", name[1], "");
						return 1;
					}
					if (datalength[1] != strlen(name[1]) + 1) {
						fprintf(stderr, "Col 2 bad length:\n'%ld'\n! =\n'%d'\n", (long) strlen(name[1]) + 1, datalength[1]);
						return 1;
					}
					if (ind[2] == 0) {
						fprintf(stderr, "Col 3 returned not NULL (ind %d len %d)\n", (int) ind[2], (int) datalength[2]);
						return 1;
					}
					if (strcmp(name[2], "")) {
						fprintf(stderr, "Col 3 bad return:\n'%s'\n! =\n'%s'\n", name[2], "");
						return 1;
					}
					if (datalength[2] != 0) {
						fprintf(stderr, "Col 3 bad length:\n'%ld'\n! =\n'%d'\n", (long) 0, datalength[2]);
						return 1;
					}
				}
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
		case CS_COMPUTE_RESULT:
			fprintf(stderr, "ct_results() unexpected CS_COMPUTE_RESULT.\n");
			return 1;
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

	return 0;
}
