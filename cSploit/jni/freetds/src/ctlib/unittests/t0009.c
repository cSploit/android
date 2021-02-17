#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: t0009.c,v 1.13 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/* Testing: Retrieve compute results */
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
	CS_INT num_cols, compute_id;

	CS_DATAFMT datafmt;
	CS_INT datalength;
	CS_SMALLINT ind;
	CS_INT count, row_count = 0;

	CS_CHAR select[1024];

	CS_INT col1;
	CS_CHAR col2[2];
	CS_CHAR col3[32];

	CS_INT compute_col1;
	CS_CHAR compute_col3[32];

	fprintf(stdout, "%s: Retrieve compute results processing\n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	ret = run_command(cmd, "CREATE TABLE #ctlib0009 (col1 int not null,  col2 char(1) not null, col3 datetime not null)");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlib0009 values (1, 'A', 'Jan  1 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlib0009 values (2, 'A', 'Jan  2 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlib0009 values (3, 'A', 'Jan  3 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlib0009 values (8, 'B', 'Jan  4 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;

	ret = run_command(cmd, "insert into #ctlib0009 values (9, 'B', 'Jan  5 2002 10:00:00AM')");
	if (ret != CS_SUCCEED)
		return 1;


	strcpy(select, "select col1, col2, col3 from #ctlib0009 order by col2 ");
	strcat(select, "compute sum(col1) by col2 ");
	strcat(select, "compute max(col3)");

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
			if (num_cols != 3) {
				fprintf(stderr, "num_cols %d != 3", num_cols);
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
			ret = ct_bind(cmd, 1, &datafmt, &col1, &datalength, &ind);
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
			datafmt.maxlength = 2;

			ret = ct_bind(cmd, 2, &datafmt, col2, &datalength, &ind);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}

			ret = ct_describe(cmd, 3, &datafmt);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_describe() failed");
				return 1;
			}

			datafmt.datatype = CS_CHAR_TYPE;
			datafmt.format = CS_FMT_NULLTERM;
			datafmt.maxlength = 32;

			ret = ct_bind(cmd, 3, &datafmt, col3, &datalength, &ind);
			if (ret != CS_SUCCEED) {
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
					fprintf(stdout, "col1 = %d col2= '%s', col3 = '%s'\n", col1, col2, col3);
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

			printf("testing compute_result\n");

			ret = ct_compute_info(cmd, CS_COMP_ID, CS_UNUSED, &compute_id, CS_UNUSED, NULL);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_compute_info() failed");
				return 1;
			}

			if (compute_id != 1 && compute_id != 2) {
				fprintf(stderr, "invalid compute_id value");
				return 1;
			}

			if (compute_id == 1) {
				ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_res_info() failed");
					return 1;
				}
				if (num_cols != 1) {
					fprintf(stderr, "compute_id %d num_cols %d != 1", compute_id, num_cols);
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
				compute_col1 = -1;
				ret = ct_bind(cmd, 1, &datafmt, &compute_col1, &datalength, &ind);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_bind() failed\n");
					return 1;
				}
			}
			if (compute_id == 2) {
				ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_res_info() failed");
					return 1;
				}
				if (num_cols != 1) {
					fprintf(stderr, "compute_id %d num_cols %d != 1", compute_id, num_cols);
					return 1;
				}

				ret = ct_describe(cmd, 1, &datafmt);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_describe() failed");
					return 1;
				}

				datafmt.datatype = CS_CHAR_TYPE;
				datafmt.format = CS_FMT_NULLTERM;
				datafmt.maxlength = 32;

				compute_col3[0] = 0;
				ret = ct_bind(cmd, 1, &datafmt, compute_col3, &datalength, &ind);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_bind() failed\n");
					return 1;
				}
			}


			while (((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) == CS_SUCCEED)
			       || (ret == CS_ROW_FAIL)) {
				row_count += count;
				if (ret == CS_ROW_FAIL) {
					fprintf(stderr, "ct_fetch() CS_ROW_FAIL on row %d.\n", row_count);
					return 1;
				} else {	/* ret == CS_SUCCEED */
					if (compute_id == 1) {
						fprintf(stdout, "compute_col1 = %d \n", compute_col1);
						if (compute_col1 != 6 && compute_col1 != 17) {
							fprintf(stderr, "(should be 6 or 17)\n");
							return 1;
						}
					}
					if (compute_id == 2) {
						fprintf(stdout, "compute_col3 = '%s'\n", compute_col3);
						if (strcmp("Jan  5 2002 10:00:00AM", compute_col3)
						    && strcmp("Jan 05 2002 10:00AM", compute_col3)
						    && strcmp("Jan  5 2002 10:00AM", compute_col3)) {
							fprintf(stderr, "(should be \"Jan  5 2002 10:00:00AM\")\n");
							return 1;
						}
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
