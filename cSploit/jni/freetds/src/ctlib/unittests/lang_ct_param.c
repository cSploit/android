#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <ctpublic.h>
#include "common.h"

static char software_version[] = "$Id: lang_ct_param.c,v 1.9 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static const char *query =
	"insert into #ctparam_lang (name,name2,age,cost,bdate,fval) values (@in1, @in2, @in3, @moneyval, @dateval, @floatval)";

CS_RETCODE ex_servermsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_SERVERMSG * errmsg);
CS_RETCODE ex_clientmsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_CLIENTMSG * errmsg);

static int insert_test(CS_CONNECTION *conn, CS_COMMAND *cmd, int useNames);

/* Testing: binding of data via ct_param */
int
main(int argc, char *argv[])
{
	int errCode = 0;
	
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	int verbose = 0;

	CS_RETCODE ret;
	CS_CHAR cmdbuf[4096];

	if (argc > 1 && (0 == strcmp(argv[1], "-v")))
		verbose = 1;

	fprintf(stdout, "%s: submit language query with variables using ct_param \n", __FILE__);
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

	strcpy(cmdbuf, "create table #ctparam_lang (id numeric identity not null, \
		name varchar(30), name2 varchar(20), age int, cost money, bdate datetime, fval float) ");

	ret = run_command(cmd, cmdbuf);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "create table failed\n");
		errCode = 1;
		goto ERR;
	}

	/* test by name */
	errCode = insert_test(conn, cmd, 1);
	/* if worked, test by position */
	if (0 == errCode)
		errCode = insert_test(conn, cmd, 0);
	query = "insert into #ctparam_lang (name,name2,age,cost,bdate,fval) values (?, ?, ?, ?, ?, ?)";
	if (0 == errCode)
		errCode = insert_test(conn, cmd, 0);

	if (verbose && (0 == errCode))
		fprintf(stdout, "lang_ct_param tests successful\n");

ERR:
	if (verbose) {
		fprintf(stdout, "Trying logout\n");
	}
	ret = try_ctlogout(ctx, conn, cmd, verbose);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "Logout failed\n");
		return 1;
	}

	return errCode;
}

static int 
insert_test(CS_CONNECTION *conn, CS_COMMAND *cmd, int useNames)
{
	CS_CONTEXT *ctx;

	CS_RETCODE ret;
	CS_INT res_type;

	CS_DATAFMT datafmt;
	CS_DATAFMT srcfmt;
	CS_DATAFMT destfmt;
	CS_INT intvar;
	CS_FLOAT floatvar;
	CS_MONEY moneyvar;
	CS_DATEREC datevar;
	char moneystring[10];
	char dummy_name[30];
	char dummy_name2[20];
	CS_INT destlen;

	/* clear table */
	run_command(cmd, "delete #ctparam_lang");

	/*
	 * Assign values to the variables used for parameter passing.
	 */

	intvar = 2;
	floatvar = 0.12;
	strcpy(dummy_name, "joe blow");
	strcpy(dummy_name2, "");
	strcpy(moneystring, "300.90");

	/*
	 * Clear and setup the CS_DATAFMT structures used to convert datatypes.
	 */

	memset(&srcfmt, 0, sizeof(CS_DATAFMT));
	srcfmt.datatype = CS_CHAR_TYPE;
	srcfmt.maxlength = strlen(moneystring);
	srcfmt.precision = 5;
	srcfmt.scale = 2;
	srcfmt.locale = NULL;

	memset(&destfmt, 0, sizeof(CS_DATAFMT));
	destfmt.datatype = CS_MONEY_TYPE;
	destfmt.maxlength = sizeof(CS_MONEY);
	destfmt.precision = 5;
	destfmt.scale = 2;
	destfmt.locale = NULL;

	/*
	 * Convert the string representing the money value
	 * to a CS_MONEY variable. Since this routine does not have the
	 * context handle, we use the property functions to get it.
	 */
	if ((ret = ct_cmd_props(cmd, CS_GET, CS_PARENT_HANDLE, &conn, CS_UNUSED, NULL)) != CS_SUCCEED) {
		fprintf(stderr, "ct_cmd_props() failed\n");
		return 1;
	}
	if ((ret = ct_con_props(conn, CS_GET, CS_PARENT_HANDLE, &ctx, CS_UNUSED, NULL)) != CS_SUCCEED) {
		fprintf(stderr, "ct_con_props() failed\n");
		return 1;
	}
	ret = cs_convert(ctx, &srcfmt, (CS_VOID *) moneystring, &destfmt, &moneyvar, &destlen);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "cs_convert() failed\n");
		return 1;
	}

	/*
	 * create the command
	 */
	if ((ret = ct_command(cmd, CS_LANG_CMD, query, strlen(query), 
		CS_UNUSED)) != CS_SUCCEED) 
	{
		fprintf(stderr, "ct_command(CS_LANG_CMD) failed\n");
		return 1;
	}

	/*
	 * Clear and setup the CS_DATAFMT structure, then pass
	 * each of the parameters for the query.
	 */
	memset(&datafmt, 0, sizeof(datafmt));
	if (useNames)
		strcpy(datafmt.name, "@in1");
	else
		datafmt.name[0] = 0;
	datafmt.maxlength = 255;
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_CHAR_TYPE;
	datafmt.status = CS_INPUTVALUE;

	/*
	 * The character string variable is filled in by the RPC so pass NULL
	 * for the data 0 for data length, and -1 for the indicator arguments.
	 */
	ret = ct_param(cmd, &datafmt, dummy_name, strlen(dummy_name), 0);
	if (CS_SUCCEED != ret) {
		fprintf(stderr, "ct_param(char) failed\n");
		return 1;
	}

	if (useNames)
		strcpy(datafmt.name, "@in2");
	else
		datafmt.name[0] = 0;
	datafmt.maxlength = 255;
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_CHAR_TYPE;
	datafmt.status = CS_INPUTVALUE;

	ret = ct_param(cmd, &datafmt, dummy_name2, strlen(dummy_name2), 0);
	if (CS_SUCCEED != ret) {
		fprintf(stderr, "ct_param(char) failed\n");
		return 1;
	}

	if (useNames)
		strcpy(datafmt.name, "@in3");
	else
		datafmt.name[0] = 0;
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_INT_TYPE;
	datafmt.status = CS_INPUTVALUE;

	ret = ct_param(cmd, &datafmt, (CS_VOID *) & intvar, CS_SIZEOF(CS_INT), 0);
	if (CS_SUCCEED != ret) {
		fprintf(stderr, "ct_param(int) failed\n");
		return 1;
	}

	if (useNames)
		strcpy(datafmt.name, "@moneyval");
	else
		datafmt.name[0] = 0;
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_MONEY_TYPE;
	datafmt.status = CS_INPUTVALUE;

	ret = ct_param(cmd, &datafmt, (CS_VOID *) & moneyvar, CS_SIZEOF(CS_MONEY), 0);
	if (CS_SUCCEED != ret) {
		fprintf(stderr, "ct_param(money) failed\n");
		return 1;
	}

	if (useNames)
		strcpy(datafmt.name, "@dateval");
	else
		datafmt.name[0] = 0;
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_DATETIME_TYPE;
	datafmt.status = CS_INPUTVALUE;
	memset(&datevar, 0, sizeof(CS_DATEREC));
	datevar.dateyear = 2003;
	datevar.datemonth = 2;
	datevar.datedmonth = 1;

	ret = ct_param(cmd, &datafmt, &datevar, 0, 0);
	if (CS_SUCCEED != ret) {
		fprintf(stderr, "ct_param(datetime) failed");
		return 1;
	}

	if (useNames)
		strcpy(datafmt.name, "@floatval");
	else
		datafmt.name[0] = 0;
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_FLOAT_TYPE;
	datafmt.status = CS_INPUTVALUE;

	ret = ct_param(cmd, &datafmt, &floatvar, 0, 0);
	if (CS_SUCCEED != ret) {
		fprintf(stderr, "ct_param(float) failed");
		return 1;
	}

	/*
	 * Send the command to the server
	 */
	if (ct_send(cmd) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(CS_LANG_CMD) failed\n");
		return 1;
	}

	/*
	 * Process the results.
	 */
	while ((ret = ct_results(cmd, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {

		case CS_CMD_SUCCEED:
		case CS_CMD_DONE:
			{
				CS_INT rowsAffected=0;
				ct_res_info(cmd, CS_ROW_COUNT, &rowsAffected, CS_UNUSED, NULL);
				if (1 != rowsAffected) 
					fprintf(stderr, "insert touched %d rows instead of 1\n", rowsAffected);
			}
			break;

		case CS_CMD_FAIL:
			/*
			 * The server encountered an error while
			 * processing our command.
			 */
			fprintf(stderr, "ct_results returned CS_CMD_FAIL.\n");
			break;

		case CS_STATUS_RESULT:
			/*
			 * The server encountered an error while
			 * processing our command.
			 */
			fprintf(stderr, "ct_results returned CS_STATUS_RESULT.\n");
			break;

		default:
			/*
			 * We got something unexpected.
			 */
			fprintf(stderr, "ct_results returned unexpected result type %d\n", res_type);
			return 1;
		}
	}
	if (ret != CS_END_RESULTS)
		fprintf(stderr, "ct_results returned unexpected result %d.\n", (int) ret);

	/* test row inserted */
	ret = run_command(cmd, "if not exists(select * from #ctparam_lang where name = 'joe blow' and name2 is not null and age = 2) select 1");
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "check row inserted failed\n");
		exit(1);
	}

	return 0;
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
