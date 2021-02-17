#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <stdlib.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: datafmt.c,v 1.5 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

/* Testing: data truncation behavior of ct_fetch */
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
	CS_INT copied = 0;
	CS_SMALLINT ind = 0;
	CS_INT count, row_count = 0;

	CS_CHAR select[1024];

	char *addr = NULL;

	fprintf(stdout, "%s: test data truncation behavior of ct_fetch \n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin_with_options(argc, argv, &ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}
	verbose += common_pwd.fverbose;

	strcpy(select, "select name from systypes where datalength(name) > 2*9 order by datalength(name)");
	printf("%s\n", select);

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
			free(addr);
			return 1;
		case CS_ROW_RESULT:

			ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_res_info() failed");
				free(addr);
				return 1;
			}
			fprintf(stderr, "%d column%s\n", num_cols, num_cols==1? "":"s");

			ret = ct_describe(cmd, 1, &datafmt);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_describe() failed\n");
				free(addr);
				return 1;
			}

			fprintf(stderr, "type = %d\n", datafmt.datatype);
			fprintf(stderr, "maxlength = %d\n", datafmt.maxlength);
			if (datafmt.datatype == CS_CHAR_TYPE) {
				fprintf(stderr, "CS_CHAR_TYPE\n");
				datafmt.format = CS_FMT_NULLTERM;
				addr = malloc(datafmt.maxlength);
			}

			fprintf(stderr, "binding column 1 (%s)\n", datafmt.name);

			/* set maxlength to something short to test truncation behavior */
			if (common_pwd.maxlength) 
				datafmt.maxlength = common_pwd.maxlength;

			ret = ct_bind(cmd, 1, &datafmt, addr, &copied, &ind);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				free(addr);
				return 1;
			}

			fprintf(stderr, "fetching rows with datafmt.maxlength = %d\n", datafmt.maxlength);

			while ((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) != CS_END_DATA) {

				fprintf(stderr, "ct_fetch() row %d returned %d.\n", row_count, (int) ret);
				addr[copied] = '\0';
				fprintf(stderr, "copied %d bytes: [%s]\n", copied, addr);
				row_count += count;

				switch (ret) {
				case CS_SUCCEED:
					fprintf(stdout, "ct_fetch returned %d row%s\n", count, count==1? "":"s");
					break;
				case CS_ROW_FAIL:
					fprintf(stderr, "error: ct_fetch() returned CS_ROW_FAIL on row %d.\n", row_count);
					free(addr);
					return 1;
				case CS_CANCELED:
					fprintf(stderr, "error: ct_fetch() returned CS_CANCELED??\n");
					free(addr);
					return 1;
				case CS_FAIL:
					fprintf(stderr, "error: ct_fetch() returned CS_FAIL.\n");
					free(addr);
					return 1;
				default:
					fprintf(stderr, "error: ct_fetch() unexpected return.\n");
					free(addr);
					return 1;
				}
			}
			
			break;
		}
	}
	
	switch ((int) results_ret) {
	case CS_END_RESULTS:
		break;
	case CS_FAIL:
		fprintf(stderr, "ct_results() failed.\n");
		free(addr);
		return 1;
		break;
	default:
		fprintf(stderr, "ct_results() unexpected return.\n");
		free(addr);
		return 1;
	}

	if (verbose) {
		fprintf(stdout, "Trying logout\n");
	}
	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		free(addr);
		return 1;
	}

	free(addr);
	return 0;
}
