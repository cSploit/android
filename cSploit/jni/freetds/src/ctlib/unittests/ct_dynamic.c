#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <stdio.h>
#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: ct_dynamic.c,v 1.4 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#define QUERY_STRING "insert into #ctparam_lang (name,age,cost,bdate,fval) values (@in1, @in2, @moneyval, @dateval, @floatval)"

CS_RETCODE ex_servermsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_SERVERMSG * errmsg);
CS_RETCODE ex_clientmsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_CLIENTMSG * errmsg);

int insert_test(CS_CONNECTION * conn, CS_COMMAND * cmd, short useNames);

int
main(int argc, char *argv[])
{
	int errCode = 0;

	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	CS_COMMAND *cmd2;
	int verbose = 0;

	CS_RETCODE ret;
	CS_RETCODE results_ret;
	CS_CHAR cmdbuf[4096];
	CS_CHAR name[257];
	CS_INT datalength;
	short int ind;
	CS_INT count;
	CS_INT num_cols;
	CS_INT row_count = 0;
	CS_DATAFMT datafmt;
	CS_DATAFMT descfmt;
	CS_INT intvar;
	CS_INT intvarsize;
	CS_SMALLINT intvarind;
	CS_INT res_type;

	int i;

	if (argc > 1 && (0 == strcmp(argv[1], "-v")))
		verbose = 1;

	fprintf(stdout, "%s: use ct_dynamic/ct_param to prepare and execute  a statement \n", __FILE__);
	if (verbose) {
		fprintf(stdout, "Trying login\n");
	}
	ret = try_ctlogin(&ctx, &conn, &cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Login failed\n");
		return 1;
	}

	ct_callback(ctx, NULL, CS_SET, CS_CLIENTMSG_CB, (CS_VOID *) ex_clientmsg_cb);

	ct_callback(ctx, NULL, CS_SET, CS_SERVERMSG_CB, (CS_VOID *) ex_servermsg_cb);

	ret = ct_cmd_alloc(conn, &cmd2);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "cmd2_alloc failed\n");
		errCode = 1;
		goto ERR;
	}

	/* do not test error */
	ret = run_command(cmd, "IF OBJECT_ID('tempdb..#ct_dynamic') IS NOT NULL DROP table #ct_dynamic");

	strcpy(cmdbuf, "create table #ct_dynamic (id numeric identity not null, \
        name varchar(30), age int, cost money, bdate datetime, fval float) ");

	ret = run_command(cmd, cmdbuf);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "create table failed\n");
		errCode = 1;
		goto ERR;
	}

	strcpy(cmdbuf, "insert into #ct_dynamic ( name , age , cost , bdate , fval ) ");
	strcat(cmdbuf, "values ('Bill', 44, 2000.00, 'May 21 1960', 60.97 ) ");

	ret = run_command(cmd, cmdbuf);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "insert table failed\n");
		errCode = 1;
		goto ERR;
	}

	strcpy(cmdbuf, "insert into #ct_dynamic ( name , age , cost , bdate , fval ) ");
	strcat(cmdbuf, "values ('Freddy', 32, 1000.00, 'Jan 21 1972', 70.97 ) ");

	ret = run_command(cmd, cmdbuf);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "insert table failed\n");
		errCode = 1;
		goto ERR;
	}

	strcpy(cmdbuf, "insert into #ct_dynamic ( name , age , cost , bdate , fval ) ");
	strcat(cmdbuf, "values ('James', 42, 5000.00, 'May 21 1962', 80.97 ) ");

	ret = run_command(cmd, cmdbuf);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "insert table failed\n");
		errCode = 1;
		goto ERR;
	}

	strcpy(cmdbuf, "select name from #ct_dynamic where age = ?");

	ret = ct_dynamic(cmd, CS_PREPARE, "age", CS_NULLTERM, cmdbuf, CS_NULLTERM);

	if (ct_send(cmd) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(CS_PREPARE) failed\n");
		errCode = 1;
		goto ERR;
	}

	while ((ret = ct_results(cmd, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {

		case CS_CMD_SUCCEED:
		case CS_CMD_DONE:
			break;

		case CS_CMD_FAIL:
			break;

		default:
			errCode = 1;
			goto ERR;
		}
	}

	if (ret != CS_END_RESULTS) {
		errCode = 1;
		goto ERR;
	}

	ret = ct_dynamic(cmd, CS_DESCRIBE_INPUT, "age", CS_NULLTERM, NULL, CS_UNUSED);
	if (ct_send(cmd) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(CS_DESCRIBE_INPUT) failed\n");
		errCode = 1;
		goto ERR;
	}

	while ((ret = ct_results(cmd, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {

		case CS_DESCRIBE_RESULT:
			ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_res_info() failed");
				return 1;
			}

			for (i = 1; i <= num_cols; i++) {
				ret = ct_describe(cmd, i, &descfmt);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_describe() failed");
					return 1;
				}
				fprintf(stderr, "CS_DESCRIBE_INPUT parameter %d :\n", i);
				if (descfmt.namelen == 0)
					fprintf(stderr, "\t\tNo name...\n");
				else
					fprintf(stderr, "\t\tName = %*.*s\n", descfmt.namelen, descfmt.namelen, descfmt.name);
				fprintf(stderr, "\t\tType   = %d\n", descfmt.datatype);
				fprintf(stderr, "\t\tLength = %d\n", descfmt.maxlength);
			}
			break;

		case CS_CMD_SUCCEED:
		case CS_CMD_DONE:
			break;

		case CS_CMD_FAIL:
			break;

		default:
			errCode = 1;
			goto ERR;
		}
	}

	ret = ct_dynamic(cmd, CS_DESCRIBE_OUTPUT, "age", CS_NULLTERM, NULL, CS_UNUSED);
	if (ct_send(cmd) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(CS_DESCRIBE_OUTPUT) failed\n");
		errCode = 1;
		goto ERR;
	}

	while ((ret = ct_results(cmd, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {

		case CS_DESCRIBE_RESULT:
			ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_res_info() failed");
				return 1;
			}
			if (num_cols != 1) {
				fprintf(stderr, "CS_DESCRIBE_OUTPUT showed %d columns , expected 1\n", num_cols);
				errCode = 1;
				goto ERR;
			}

			for (i = 1; i <= num_cols; i++) {
				ret = ct_describe(cmd, i, &descfmt);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_describe() failed");
					return 1;
				}
				if (descfmt.namelen == 0)
					fprintf(stderr, "\t\tNo name...\n");
				else
					fprintf(stderr, "\t\tName = %*.*s\n", descfmt.namelen, descfmt.namelen, descfmt.name);
				fprintf(stderr, "\t\tType   = %d\n", descfmt.datatype);
				fprintf(stderr, "\t\tLength = %d\n", descfmt.maxlength);
			}
			break;

		case CS_CMD_SUCCEED:
		case CS_CMD_DONE:
			break;

		case CS_CMD_FAIL:
			break;

		default:
			errCode = 1;
			goto ERR;
		}
	}
	ret = ct_dynamic(cmd2, CS_EXECUTE, "age", CS_NULLTERM, NULL, CS_UNUSED);

	intvar = 44;
	intvarsize = 4;
	intvarind = 0;

	datafmt.name[0] = 0;
	datafmt.namelen = 0;
	datafmt.datatype = CS_INT_TYPE;
	datafmt.status = CS_INPUTVALUE;

	ret = ct_setparam(cmd2, &datafmt, (CS_VOID *) & intvar, &intvarsize, &intvarind);
	if (CS_SUCCEED != ret) {
		fprintf(stderr, "ct_setparam(int) failed\n");
		return 1;
	}

	if (ct_send(cmd2) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(CS_EXECUTE) failed\n");
		errCode = 1;
		goto ERR;
	}

	while ((results_ret = ct_results(cmd2, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {
		case CS_CMD_SUCCEED:
			break;
		case CS_CMD_DONE:
			break;
		case CS_CMD_FAIL:
			fprintf(stderr, "1: ct_results() result_type CS_CMD_FAIL.\n");
			return 1;
		case CS_ROW_RESULT:
			datafmt.datatype = CS_CHAR_TYPE;
			datafmt.format = CS_FMT_NULLTERM;
			datafmt.maxlength = 256;
			datafmt.count = 1;
			datafmt.locale = NULL;
			ret = ct_bind(cmd2, 1, &datafmt, name, &datalength, &ind);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}

			while (((ret = ct_fetch(cmd2, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) == CS_SUCCEED)
			       || (ret == CS_ROW_FAIL)) {
				row_count += count;
				if (ret == CS_ROW_FAIL) {
					fprintf(stderr, "ct_fetch() CS_ROW_FAIL on row %d.\n", row_count);
					return 1;
				} else if (ret == CS_SUCCEED) {
					if (strcmp(name, "Bill")) {
						fprintf(stderr, "fetched value '%s' expected 'Bill'\n", name);
						errCode = 1;
						goto ERR;
					}
				} else {
					break;
				}
			}
			switch ((int) ret) {
			case CS_END_DATA:
				break;
			case CS_FAIL:
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

	intvar = 32;

	if (ct_send(cmd2) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(CS_EXECUTE) failed\n");
		errCode = 1;
		goto ERR;
	}

	while ((results_ret = ct_results(cmd2, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {
		case CS_CMD_SUCCEED:
			break;
		case CS_CMD_DONE:
			break;
		case CS_CMD_FAIL:
			fprintf(stderr, "2: ct_results() result_type CS_CMD_FAIL.\n");
			return 1;
		case CS_ROW_RESULT:
			datafmt.datatype = CS_CHAR_TYPE;
			datafmt.format = CS_FMT_NULLTERM;
			datafmt.maxlength = 256;
			datafmt.count = 1;
			datafmt.locale = NULL;
			ret = ct_bind(cmd2, 1, &datafmt, name, &datalength, &ind);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_bind() failed\n");
				return 1;
			}

			while (((ret = ct_fetch(cmd2, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count)) == CS_SUCCEED)
			       || (ret == CS_ROW_FAIL)) {
				row_count += count;
				if (ret == CS_ROW_FAIL) {
					fprintf(stderr, "ct_fetch() CS_ROW_FAIL on row %d.\n", row_count);
					return 1;
				} else if (ret == CS_SUCCEED) {
					if (strcmp(name, "Freddy")) {
						fprintf(stderr, "fetched value '%s' expected 'Freddy'\n", name);
						errCode = 1;
						goto ERR;
					}
				} else {
					break;
				}
			}
			switch ((int) ret) {
			case CS_END_DATA:
				break;
			case CS_FAIL:
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

	ret = ct_dynamic(cmd, CS_DEALLOC, "age", CS_NULLTERM, NULL, CS_UNUSED);

	if (ct_send(cmd) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(CS_DEALLOC) failed\n");
		errCode = 1;
		goto ERR;
	}

	while ((ret = ct_results(cmd, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {

		case CS_CMD_SUCCEED:
		case CS_CMD_DONE:
			break;

		case CS_CMD_FAIL:
			break;

		default:
			errCode = 1;
			goto ERR;
		}
	}

	if (ret != CS_END_RESULTS) {
		errCode = 1;
		goto ERR;
	}
      ERR:
	if (verbose) {
		fprintf(stdout, "Trying logout\n");
	}

	ct_cmd_drop(cmd2);

	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 1;
	}

	return errCode;
}


CS_RETCODE
ex_clientmsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_CLIENTMSG * errmsg)
{
	fprintf(stdout, "\nOpen Client Message:\n");
	fprintf(stdout, "Message number: LAYER = (%d) ORIGIN = (%d) ", CS_LAYER(errmsg->msgnumber), CS_ORIGIN(errmsg->msgnumber));
	fprintf(stdout, "SEVERITY = (%d) NUMBER = (%d)\n", CS_SEVERITY(errmsg->msgnumber), CS_NUMBER(errmsg->msgnumber));
	fprintf(stdout, "Message String: %s\n", errmsg->msgstring);
	if (errmsg->osstringlen > 0) {
		fprintf(stdout, "Operating System Error: %s\n", errmsg->osstring);
	}
	fflush(stdout);

	return CS_SUCCEED;
}

CS_RETCODE
ex_servermsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_SERVERMSG * srvmsg)
{
	fprintf(stdout, "\nServer message:\n");
	fprintf(stdout, "Message number: %d, Severity %d, ", srvmsg->msgnumber, srvmsg->severity);
	fprintf(stdout, "State %d, Line %d\n", srvmsg->state, srvmsg->line);

	if (srvmsg->svrnlen > 0) {
		fprintf(stdout, "Server '%s'\n", srvmsg->svrname);
	}

	if (srvmsg->proclen > 0) {
		fprintf(stdout, " Procedure '%s'\n", srvmsg->proc);
	}

	fprintf(stdout, "Message String: %s\n", srvmsg->text);
	fflush(stdout);

	return CS_SUCCEED;
}
