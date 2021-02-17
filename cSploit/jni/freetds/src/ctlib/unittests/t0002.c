#include <config.h>

#include <stdio.h>
#include <string.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: t0002.c,v 1.10 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int sp_who(CS_COMMAND *cmd);

/*
 * ct_send SQL |select name = @@servername|
 * ct_bind variable
 * ct_fetch and print results
 */
int
main(int argc, char **argv)
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	int verbose = 0;

	CS_RETCODE ret;
	CS_RETCODE results_ret;
	CS_DATAFMT datafmt;
	CS_INT datalength;
	CS_SMALLINT ind;
	CS_INT count, row_count = 0;
	CS_INT result_type;
	CS_CHAR name[256];

	fprintf(stdout, "%s: Testing bind & select\n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	ret = ct_command(cmd, CS_LANG_CMD, "select name = @@servername", CS_NULLTERM, CS_UNUSED);
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
			datafmt.datatype = CS_CHAR_TYPE;
			datafmt.format = CS_FMT_NULLTERM;
			datafmt.maxlength = 256;
			datafmt.count = 1;
			datafmt.locale = NULL;
			ret = ct_bind(cmd, 1, &datafmt, name, &datalength, &ind);
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
				} else if (ret == CS_SUCCEED) {
					if (verbose) {
						fprintf(stdout, "server name = %s\n", name);
					}
				} else {
					break;
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

	/*
	 * test parameter return code processing with sp_who
	 */
	sp_who(cmd);

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

int 
sp_who(CS_COMMAND *cmd)
{
	enum {maxcol=10, colsize=260};
	
	struct _col { 
		CS_DATAFMT datafmt;
		CS_INT datalength;
		CS_SMALLINT ind;
		CS_CHAR data[colsize];
	} col[maxcol];
	
	CS_INT num_cols;
	CS_INT count, row_count = 0;
	CS_INT result_type;
	CS_RETCODE ret;
	CS_RETCODE results_ret;
	int i;
	int is_status_result=0;

	ret = ct_command(cmd, CS_LANG_CMD, "exec sp_who", CS_NULLTERM, CS_UNUSED);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_command: \"exec sp_who\" failed with %d\n", ret);
		return 1;
	}
	ret = ct_send(cmd);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ct_send: \"exec sp_who\" failed with %d\n", ret);
		return 1;
	}
	while ((results_ret = ct_results(cmd, &result_type)) == CS_SUCCEED) {
		is_status_result = 0;
		switch ((int) result_type) {
		case CS_CMD_SUCCEED:
			break;
		case CS_CMD_DONE:
			break;
		case CS_CMD_FAIL:
			fprintf(stderr, "ct_results() result_type CS_CMD_FAIL.\n");
			return 1;
		case CS_STATUS_RESULT:
			fprintf(stdout, "ct_results: CS_STATUS_RESULT detected for sp_who\n");
			is_status_result = 1;
			/* fall through */
		case CS_ROW_RESULT:
			ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
			if (ret != CS_SUCCEED || num_cols > maxcol) {
				fprintf(stderr, "ct_res_info() failed\n");
				return 1;
			}

			if (num_cols <= 0) {
				fprintf(stderr, "ct_res_info() return strange values\n");
				return 1;
			}

			if (is_status_result && num_cols != 1) {
				fprintf(stderr, "CS_STATUS_RESULT return more than 1 column\n");
				return 1;
			}
			
			for (i=0; i < num_cols; i++) {

				/* here we can finally test for the return status column */
				ret = ct_describe(cmd, i+1, &col[i].datafmt);
				
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_describe() failed for column %d\n", i);
					return 1;
				}

				if (col[i].datafmt.status & CS_RETURN) {
					fprintf(stdout, "ct_describe() indicates a return code in column %d for sp_who\n", i);
					
					/*
					 * other possible values:
					 * CS_CANBENULL
					 * CS_HIDDEN
					 * CS_IDENTITY
					 * CS_KEY
					 * CS_VERSION_KEY
					 * CS_TIMESTAMP
					 * CS_UPDATABLE
					 * CS_UPDATECOL
					 */
				}
				
				col[i].datafmt.datatype = CS_CHAR_TYPE;
				col[i].datafmt.format = CS_FMT_NULLTERM;
				col[i].datafmt.maxlength = colsize;
				col[i].datafmt.count = 1;
				col[i].datafmt.locale = NULL;

				ret = ct_bind(cmd, i+1, &col[i].datafmt, &col[i].data, &col[i].datalength, &col[i].ind);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_bind() failed\n");
					return 1;
				}
				
			}

			row_count = 0;
			while ((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) == CS_SUCCEED) {
				if( row_count == 0) { /* titles */
					for (i=0; i < num_cols; i++) {
						char fmt[40];
						if (col[i].datafmt.namelen == 0) {
							printf("unnamed%d ",i+1);
							continue;
						}
						sprintf(fmt, "%%-%d.%ds  ", col[i].datafmt.namelen, col[i].datafmt.maxlength);
						printf(fmt, col[i].datafmt.name);
					}
					printf("\n");
				}
				
				for (i=0; i < num_cols; i++) { /* data */
					char fmt[40];
					if (col[i].ind)
						continue;
					sprintf(fmt, "%%-%d.%ds  ", col[i].datalength, col[i].datafmt.maxlength);
					printf(fmt, col[i].data);
					if (is_status_result && strcmp(col[i].data,"0")) {
						fprintf(stderr, "CS_STATUS_RESULT should return 0 as result\n");
						return 1;
					}
				}

				printf("\n");

				row_count += count;
			}
			if (is_status_result && row_count != 1) {
				fprintf(stderr, "CS_STATUS_RESULT should return a row\n");
				return 1;
			}
			
			switch ((int) ret) {
			case CS_END_DATA:
				fprintf(stdout, "ct_results fetched %d rows.\n", row_count);
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
	
	return 0;
}
