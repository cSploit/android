#include <config.h>

#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <ctpublic.h>
#include "common.h"

#define MAX(X,Y)      (((X) > (Y)) ? (X) : (Y))
#define MIN(X,Y)      (((X) < (Y)) ? (X) : (Y))

static char software_version[] = "$Id: rpc_ct_setparam.c,v 1.11 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

CS_RETCODE ex_clientmsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_CLIENTMSG * errmsg);
CS_RETCODE ex_servermsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_SERVERMSG * errmsg);
static CS_RETCODE ex_display_header(CS_INT numcols, CS_DATAFMT columns[]);
static CS_INT ex_display_dlen(CS_DATAFMT *column);
static CS_INT ex_display_results(CS_COMMAND * cmd);

typedef struct _ex_column_data
{
	CS_SMALLINT indicator;
	CS_CHAR *value;
	CS_INT valuelen;
}
EX_COLUMN_DATA;

/* Testing: array binding of result set */
int
main(int argc, char *argv[])
{
	CS_CONTEXT *ctx;
	CS_CONNECTION *conn;
	CS_COMMAND *cmd;
	int verbose = 0;

	CS_RETCODE ret;

	CS_INT datalength;
	CS_SMALLINT nullind;
	CS_SMALLINT notnullind;

	CS_CHAR cmdbuf[4096];

	CS_DATAFMT datafmt;
	CS_DATAFMT srcfmt;
	CS_DATAFMT destfmt;
	CS_INT intvar;
	CS_SMALLINT smallintvar;
	CS_FLOAT floatvar;
	CS_MONEY moneyvar;
	CS_BINARY binaryvar;
	char moneystring[10];
	char rpc_name[15];
	CS_INT destlen;



	fprintf(stdout, "%s: submit a stored procedure using ct_setparam \n", __FILE__);
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

	/* do not test error */
	ret = run_command(cmd, "IF EXISTS(SELECT * FROM SYSOBJECTS WHERE name = 'sample_rpc' AND type = 'P') DROP PROCEDURE sample_rpc");

	strcpy(cmdbuf, "create proc sample_rpc (@intparam int, \
        @sintparam smallint output, @floatparam float output, \
        @moneyparam money output,  \
        @dateparam datetime output, @charparam char(20) output, \
        @binaryparam    varbinary(2000) output) \
        as ");

	strcat(cmdbuf, "select @intparam, @sintparam, @floatparam, @moneyparam, \
        @dateparam, @charparam, @binaryparam \
        select @sintparam = @sintparam + @intparam \
        select @floatparam = @floatparam + @intparam \
        select @moneyparam = @moneyparam + convert(money, @intparam) \
        select @dateparam = getdate() \
        select @charparam = \'The char parameters\' \
        select @binaryparam = @binaryparam \
        print \'This is the message printed out by sample_rpc.\'");

	ret = run_command(cmd, cmdbuf);

	if (ret != CS_SUCCEED) {
		fprintf(stderr, "create proc failed\n");
		return 1;
	}

	/*
	 * Assign values to the variables used for parameter passing.
	 */

	intvar = 2;
	smallintvar = 234;
	floatvar = 0.12;
	binaryvar = (CS_BINARY) 0xff;
	strcpy(rpc_name, "sample_rpc");
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
		fprintf(stderr, "ct_cmd_props() failed");
		return 1;
	}
	if ((ret = ct_con_props(conn, CS_GET, CS_PARENT_HANDLE, &ctx, CS_UNUSED, NULL)) != CS_SUCCEED) {
		fprintf(stderr, "ct_con_props() failed");
		return 1;
	}
	ret = cs_convert(ctx, &srcfmt, (CS_VOID *) moneystring, &destfmt, &moneyvar, &destlen);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "cs_convert() failed");
		return 1;
	}

	/*
	 * Send the RPC command for our stored procedure.
	 */
	if ((ret = ct_command(cmd, CS_RPC_CMD, rpc_name, CS_NULLTERM, CS_NO_RECOMPILE)) != CS_SUCCEED) {
		fprintf(stderr, "ct_command(CS_RPC_CMD) failed");
		return 1;
	}

	nullind    = -1;
	notnullind = 0;
	/*
	 * Clear and setup the CS_DATAFMT structure, then pass
	 * each of the parameters for the RPC.
	 */
	memset(&datafmt, 0, sizeof(datafmt));
	strcpy(datafmt.name, "@intparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_INT_TYPE;
	datafmt.maxlength = CS_UNUSED;
	datafmt.status = CS_INPUTVALUE;
	datafmt.locale = NULL;

	datalength = CS_SIZEOF(CS_INT);

	if ((ret = ct_setparam(cmd, &datafmt, (CS_VOID *) & intvar, &datalength, &notnullind)) != CS_SUCCEED) {
		fprintf(stderr, "ct_setparam(int) failed");
		return 1;
	}

	strcpy(datafmt.name, "@sintparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_SMALLINT_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	datalength = CS_SIZEOF(CS_SMALLINT);

	if ((ret = ct_setparam(cmd, &datafmt, (CS_VOID *) & smallintvar, &datalength, &notnullind)) != CS_SUCCEED) {
		fprintf(stderr, "ct_setparam(smallint) failed");
		return 1;
	}

	strcpy(datafmt.name, "@floatparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_FLOAT_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	datalength = CS_SIZEOF(CS_FLOAT);

	if ((ret = ct_setparam(cmd, &datafmt, (CS_VOID *) & floatvar, &datalength, &notnullind)) != CS_SUCCEED) {
		fprintf(stderr, "ct_setparam(float) failed");
		return 1;
	}


	strcpy(datafmt.name, "@moneyparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_MONEY_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	datalength = CS_SIZEOF(CS_MONEY);

	if ((ret = ct_setparam(cmd, &datafmt, (CS_VOID *) & moneyvar, &datalength, &notnullind)) != CS_SUCCEED) {
		fprintf(stderr, "ct_setparam(money) failed");
		return 1;
	}

	strcpy(datafmt.name, "@dateparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_DATETIME4_TYPE;
	datafmt.maxlength = 255;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	/*
	 * The datetime variable is filled in by the RPC so pass NULL for
	 * the data, 0 for data length, and -1 for the indicator arguments.
	 */

	datalength = 0;

	if ((ret = ct_setparam(cmd, &datafmt, NULL, &datalength, &nullind)) != CS_SUCCEED) {
		fprintf(stderr, "ct_setparam(datetime4) failed");
		return 1;
	}
	strcpy(datafmt.name, "@charparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_CHAR_TYPE;
	datafmt.maxlength = 60;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;


	datalength = 0;

	/*
	 * The character string variable is filled in by the RPC so pass NULL
	 * for the data 0 for data length, and -1 for the indicator arguments.
	 */
	if ((ret = ct_setparam(cmd, &datafmt, NULL, &datalength, &nullind)) != CS_SUCCEED) {
		fprintf(stderr, "ct_setparam(char) failed");
		return 1;
	}

	strcpy(datafmt.name, "@binaryparam");
	datafmt.namelen = CS_NULLTERM;
	datafmt.datatype = CS_LONGBINARY_TYPE;
	datafmt.maxlength = 2000;
	datafmt.status = CS_RETURN;
	datafmt.locale = NULL;

	datalength = CS_SIZEOF(CS_BINARY);
	nullind = -1;

	if ((ret = ct_setparam(cmd, &datafmt, (CS_VOID *) & binaryvar, &datalength, &notnullind)) != CS_SUCCEED) {
		fprintf(stderr, "ct_setparam(binary) failed");
		return 1;
	}

	/*
	 * Send the command to the server
	 */
	if (ct_send(cmd) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(RPC) failed");
		return 1;
	}

	ret = ex_display_results(cmd);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ex_display_results failed\n");
		return 1;
	}

	intvar = 3;

	if (ct_send(cmd) != CS_SUCCEED) {
		fprintf(stderr, "ct_send(RPC) failed");
		return 1;
	}

	ret = ex_display_results(cmd);
	if (ret != CS_SUCCEED) {
		fprintf(stderr, "ex_display_results failed\n");
		return 1;
	}

	run_command(cmd, "DROP PROCEDURE sample_rpc");

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

static CS_INT
ex_display_results(CS_COMMAND * cmd)
{

CS_RETCODE ret;
CS_INT res_type;
CS_INT num_cols;
EX_COLUMN_DATA *coldata;
CS_DATAFMT *outdatafmt;
CS_INT row_count = 0;
CS_INT rows_read;
CS_INT disp_len;
CS_SMALLINT msg_id;
int i, j;

	/*
	 * Process the results of the RPC.
	 */
	while ((ret = ct_results(cmd, &res_type)) == CS_SUCCEED) {
		switch ((int) res_type) {
		case CS_ROW_RESULT:
		case CS_PARAM_RESULT:
		case CS_STATUS_RESULT:
			/*
			 * Print the result header based on the result type.
			 */
			switch ((int) res_type) {
			case CS_ROW_RESULT:
				fprintf(stdout, "\nROW RESULTS\n");
				break;

			case CS_PARAM_RESULT:
				fprintf(stdout, "\nPARAMETER RESULTS\n");
				break;

			case CS_STATUS_RESULT:
				fprintf(stdout, "\nSTATUS RESULTS\n");
				break;
			}
			fflush(stdout);

			/*
			 * All three of these result types are fetchable.
			 * Since the result model for rpcs and rows have
			 * been unified in the New Client-Library, we
			 * will use the same routine to display them
			 */

			/*
			 * Find out how many columns there are in this result set.
			 */
			ret = ct_res_info(cmd, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_res_info(CS_NUMDATA) failed");
				return 1;
			}

			/*
			 * Make sure we have at least one column
			 */
			if (num_cols <= 0) {
				fprintf(stderr, "ct_res_info(CS_NUMDATA) returned zero columns");
				return 1;
			}

			/*
			 * Our program variable, called 'coldata', is an array of
			 * EX_COLUMN_DATA structures. Each array element represents
			 * one column.  Each array element will re-used for each row.
			 * 
			 * First, allocate memory for the data element to process.
			 */
			coldata = (EX_COLUMN_DATA *) malloc(num_cols * sizeof(EX_COLUMN_DATA));
			if (coldata == NULL) {
				fprintf(stderr, "malloc coldata failed \n");
				return 1;
			}

			outdatafmt = (CS_DATAFMT *) malloc(num_cols * sizeof(CS_DATAFMT));
			if (outdatafmt == NULL) {
				fprintf(stderr, "malloc outdatafmt failed \n");
				return 1;
			}

			for (i = 0; i < num_cols; i++) {
				ret = ct_describe(cmd, (i + 1), &outdatafmt[i]);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_describe failed \n");
					return 1;
				}

				outdatafmt[i].maxlength = ex_display_dlen(&outdatafmt[i]) + 1;
				outdatafmt[i].datatype = CS_CHAR_TYPE;
				outdatafmt[i].format = CS_FMT_NULLTERM;

				coldata[i].value = (CS_CHAR *) malloc(outdatafmt[i].maxlength);
				coldata[i].value[0] = 0;
				if (coldata[i].value == NULL) {
					fprintf(stderr, "malloc coldata.value failed \n");
					return 1;
				}

				ret = ct_bind(cmd, (i + 1), &outdatafmt[i], coldata[i].value, &coldata[i].valuelen,
					      & coldata[i].indicator);
				if (ret != CS_SUCCEED) {
					fprintf(stderr, "ct_bind failed \n");
					return 1;
				}
			}
			if (ret != CS_SUCCEED) {
				for (j = 0; j < i; j++) {
					free(coldata[j].value);
				}
				free(coldata);
				free(outdatafmt);
				return 1;
			}

			ex_display_header(num_cols, outdatafmt);

			while (((ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED,
						&rows_read)) == CS_SUCCEED) || (ret == CS_ROW_FAIL)) {
				/*
				 * Increment our row count by the number of rows just fetched.
				 */
				row_count = row_count + rows_read;

				/*
				 * Check if we hit a recoverable error.
				 */
				if (ret == CS_ROW_FAIL) {
					fprintf(stdout, "Error on row %d.\n", row_count);
					fflush(stdout);
				}

				/*
				 * We have a row.  Loop through the columns displaying the
				 * column values.
				 */
				for (i = 0; i < num_cols; i++) {
					/*
					 * Display the column value
					 */
					fprintf(stdout, "%s", coldata[i].value);
					fflush(stdout);

					/*
					 * If not last column, Print out spaces between this
					 * column and next one.
					 */
					if (i != num_cols - 1) {
						disp_len = ex_display_dlen(&outdatafmt[i]);
						disp_len -= coldata[i].valuelen - 1;
						for (j = 0; j < disp_len; j++) {
							fputc(' ', stdout);
						}
					}
				}
				fprintf(stdout, "\n");
				fflush(stdout);
			}

			/*
			 * Free allocated space.
			 */
			for (i = 0; i < num_cols; i++) {
				free(coldata[i].value);
			}
			free(coldata);
			free(outdatafmt);

			/*
			 * We're done processing rows.  Let's check the final return
			 * value of ct_fetch().
			 */
			switch ((int) ret) {
			case CS_END_DATA:
				/*
				 * Everything went fine.
				 */
				fprintf(stdout, "All done processing rows.\n");
				fflush(stdout);
				break;

			case CS_FAIL:
				/*
				 * Something terrible happened.
				 */
				fprintf(stderr, "ct_fetch returned CS_FAIL\n");
				return 1;
				break;

			default:
				/*
				 * We got an unexpected return value.
				 */
				fprintf(stderr, "ct_fetch returned %d\n", ret);
				return 1;
				break;

			}
			break;

		case CS_MSG_RESULT:
			ret = ct_res_info(cmd, CS_MSGTYPE, (CS_VOID *) & msg_id, CS_UNUSED, NULL);
			if (ret != CS_SUCCEED) {
				fprintf(stderr, "ct_res_info(msg_id) failed");
				return 1;
			}
			fprintf(stdout, "ct_result returned CS_MSG_RESULT where msg id = %d.\n", msg_id);
			fflush(stdout);
			break;

		case CS_CMD_SUCCEED:
			/*
			 * This means no rows were returned.
			 */
			break;

		case CS_CMD_DONE:
			/*
			 * Done with result set.
			 */
			break;

		case CS_CMD_FAIL:
			/*
			 * The server encountered an error while
			 * processing our command.
			 */
			fprintf(stderr, "ct_results returned CS_CMD_FAIL.");
			break;

		default:
			/*
			 * We got something unexpected.
			 */
			fprintf(stderr, "ct_results returned unexpected result type.");
			return CS_FAIL;
		}
	}

	/*
	 * We're done processing results. Let's check the
	 * return value of ct_results() to see if everything
	 * went ok.
	 */
	switch ((int) ret) {
	case CS_END_RESULTS:
		/*
		 * Everything went fine.
		 */
		break;

	case CS_FAIL:
		/*
		 * Something failed happened.
		 */
		fprintf(stderr, "ct_results failed.");
		break;

	default:
		/*
		 * We got an unexpected return value.
		 */
		fprintf(stderr, "ct_results returned unexpected result type.");
		break;
	}

	return CS_SUCCEED;



}

static CS_INT
ex_display_dlen(CS_DATAFMT *column)
{
CS_INT len;

	switch ((int) column->datatype) {
	case CS_CHAR_TYPE:
	case CS_VARCHAR_TYPE:
	case CS_TEXT_TYPE:
	case CS_IMAGE_TYPE:
		len = MIN(column->maxlength, 1024);
		break;

	case CS_BINARY_TYPE:
	case CS_VARBINARY_TYPE:
		len = MIN((2 * column->maxlength) + 2, 1024);
		break;

	case CS_BIT_TYPE:
	case CS_TINYINT_TYPE:
		len = 3;
		break;

	case CS_SMALLINT_TYPE:
		len = 6;
		break;

	case CS_INT_TYPE:
		len = 11;
		break;

	case CS_REAL_TYPE:
	case CS_FLOAT_TYPE:
		len = 20;
		break;

	case CS_MONEY_TYPE:
	case CS_MONEY4_TYPE:
		len = 24;
		break;

	case CS_DATETIME_TYPE:
	case CS_DATETIME4_TYPE:
		len = 30;
		break;

	case CS_NUMERIC_TYPE:
	case CS_DECIMAL_TYPE:
		len = (CS_MAX_PREC + 2);
		break;

	default:
		len = 12;
		break;
	}

	return MAX((CS_INT) (strlen(column->name) + 1), len);
}

static CS_RETCODE
ex_display_header(CS_INT numcols, CS_DATAFMT columns[])
{
CS_INT i;
CS_INT l;
CS_INT j;
CS_INT disp_len;

	fputc('\n', stdout);
	for (i = 0; i < numcols; i++) {
		disp_len = ex_display_dlen(&columns[i]);
		fprintf(stdout, "%s", columns[i].name);
		fflush(stdout);
		l = disp_len - strlen(columns[i].name);
		for (j = 0; j < l; j++) {
			fputc(' ', stdout);
			fflush(stdout);
		}
	}
	fputc('\n', stdout);
	fflush(stdout);
	for (i = 0; i < numcols; i++) {
		disp_len = ex_display_dlen(&columns[i]);
		l = disp_len - 1;
		for (j = 0; j < l; j++) {
			fputc('-', stdout);
		}
		fputc(' ', stdout);
	}
	fputc('\n', stdout);

	return CS_SUCCEED;
}

CS_RETCODE
ex_clientmsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_CLIENTMSG * errmsg)
{
	fprintf(stdout, "\nOpen Client Message:\n");
	fprintf(stdout, "Message number: LAYER = (%ld) ORIGIN = (%ld) ", (long) CS_LAYER(errmsg->msgnumber), (long) CS_ORIGIN(errmsg->msgnumber));
	fprintf(stdout, "SEVERITY = (%ld) NUMBER = (%ld)\n", (long) CS_SEVERITY(errmsg->msgnumber), (long) CS_NUMBER(errmsg->msgnumber));
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
	fprintf(stdout, "Message number: %ld, Severity %ld, ", (long) srvmsg->msgnumber, (long) srvmsg->severity);
	fprintf(stdout, "State %ld, Line %ld\n", (long) srvmsg->state, (long) srvmsg->line);

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
