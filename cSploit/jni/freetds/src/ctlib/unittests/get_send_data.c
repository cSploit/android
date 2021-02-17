#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: get_send_data.c,v 1.6 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static CS_CONNECTION *conn = NULL;

/* Testing: Retrieve CS_TEXT_TYPE using ct_bind() */
int
main(int argc, char **argv)
{
	CS_CONTEXT *ctx;
	CS_COMMAND *cmd;
	int i, verbose = 0;

	CS_RETCODE ret;
	CS_RETCODE ret2;
	CS_RETCODE results_ret;
	CS_INT result_type;
	CS_INT num_cols;

	CS_DATAFMT datafmt;
	CS_INT datalength;
	CS_SMALLINT ind;
	CS_INT count, row_count = 0;

	CS_INT  id;
	CS_CHAR name[600];
	CS_CHAR *nameptr;
	CS_INT  getlen;

	char large_sql[1024];
	char len600[601];
	char len800[801];
	char temp[11];

	char *textptr;
	CS_IODESC iodesc;

	len600[0] = 0;
	name[0] = 0;
	for (i = 0; i < 60; i++) {
		sprintf(temp, "_abcde_%03d", (i + 1) * 10);
		strcat(len600, temp);
	}
	len600[600] = '\0';

	len800[0] = 0;
	for (i = 0; i < 80; i++) {
		sprintf(temp, "_zzzzz_%03d", (i + 1) * 10);
		strcat(len800, temp);
	}
	len800[800] = '\0';


	fprintf(stdout, "%s: Retrieve CS_TEXT_TYPE using ct_bind()\n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	ret = run_command(cmd, "CREATE TABLE #test_table (id int, name text)");
	if (ret != CS_SUCCEED)
		return 1;

	sprintf(large_sql, "INSERT #test_table (id, name) VALUES (2, '%s')", len600);
	ret = run_command(cmd, large_sql);
	if (ret != CS_SUCCEED)
		return 1;

	ret = ct_command(cmd, CS_LANG_CMD, "SELECT id, name FROM #test_table", CS_NULLTERM, CS_UNUSED);
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
			ret = ct_bind(cmd, 1, &datafmt, &id, &datalength, &ind);
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
					if (verbose) {
						fprintf(stdout, "id = '%d'\n", id);
					}

                    nameptr = name;
                    while ((ret2 = ct_get_data(cmd, 2 , nameptr, 200, &getlen )) == CS_SUCCEED) {
                        nameptr += getlen;
					}
                    if (ret2 != CS_END_DATA) {
						fprintf(stderr, "ct_get_data() failed\n");
						return 1;
					}

					if (memcmp(name, len600, 600)) {
						fprintf(stderr, "Bad return data\n");
						return 1;
					}
					fprintf(stdout, "%s: Trying ct_data_info on text column\n", __FILE__);

                    if (ct_data_info(cmd, CS_GET, 2, &iodesc) != CS_SUCCEED) {
						fprintf(stderr, "ct_data_info() failed\n");
						return 1;
					} else {
						fprintf(stdout, "datatype = %d\n", iodesc.datatype);
						fprintf(stdout, "usertype = %d\n", iodesc.usertype);
						fprintf(stdout, "text len = %d\n", iodesc.total_txtlen);
						fprintf(stdout, "name     = %*.*s\n", iodesc.namelen, iodesc.namelen, iodesc.name);
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

	if ((ret = ct_command(cmd, CS_SEND_DATA_CMD, NULL, CS_UNUSED, CS_COLUMN_DATA)) != CS_SUCCEED) {
		fprintf(stderr, "ct_command(CS_SEND_DATA_CMD) failed.\n");
		return 1;
	}               

	iodesc.total_txtlen = 800;
	iodesc.log_on_update = CS_TRUE;

	if (ct_data_info(cmd, CS_SET, CS_UNUSED, &iodesc) != CS_SUCCEED) {
		fprintf(stderr, "ct_data_info() failed\n");
		return 1;
	}

	for ( i = 0 ; i < 800 ; i += 200 ) {
		textptr = &len800[i];

		ret = ct_send_data(cmd, textptr, (CS_INT) 200);
		if (ret != CS_SUCCEED) {
			fprintf(stderr, "ct_send_data failed\n");
			return 1;
		}
	}
	ret = ct_send(cmd);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_send failed\n");
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
			break;
		case CS_PARAM_RESULT:
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
