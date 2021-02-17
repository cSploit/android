#include <stdio.h>
#include <ctpublic.h>

void execute(CS_COMMAND *cmd, char *id, CS_DATAFMT *datafmt, char *p1, char *p2,
			 double p3, double p4) ;
int main() 
{
CS_CONTEXT *ctx; 
CS_CONNECTION *conn; 
CS_COMMAND *cmd; 
CS_RETCODE ret;
CS_RETCODE restype;
CS_DATAFMT datafmt[10];

   ret = cs_ctx_alloc(CS_VERSION_100, &ctx);
   ret = ct_init(ctx, CS_VERSION_100);
   ret = ct_con_alloc(ctx, &conn);
   ret = ct_con_props(conn, CS_SET, CS_USERNAME, "guest", CS_NULLTERM, NULL);
   ret = ct_con_props(conn, CS_SET, CS_PASSWORD, "sybase", CS_NULLTERM, NULL);
   /* ret = ct_con_props(conn, CS_SET, CS_IFILE, "/devl/t3624bb/myinterf", CS_NULLTERM, NULL); */
   ret = ct_connect(conn, "JDBC", CS_NULLTERM);
   ret = ct_cmd_alloc(conn, &cmd);

   ret = ct_command(cmd, CS_LANG_CMD, "drop table tempdb..prepare_bug ", CS_NULLTERM, CS_UNUSED);
   if (ret != CS_SUCCEED) {
     fprintf(stderr, "ct_command() failed\n");
     exit(1);
   }
   ret = ct_send(cmd);
   if (ret != CS_SUCCEED) {
     fprintf(stderr, "ct_send() failed\n");
     exit(1);
   }

	while(ct_results(cmd, &restype) == CS_SUCCEED) ;
   ret = ct_command(cmd, CS_LANG_CMD, "create table tempdb..prepare_bug (c1 char(20), c2 varchar(255), p1 float, p2 real) ", CS_NULLTERM, CS_UNUSED);
   if (ret != CS_SUCCEED) {
     fprintf(stderr, "ct_command() failed\n");
     exit(1);
   }
   ret = ct_send(cmd);
   if (ret != CS_SUCCEED) {
     fprintf(stderr, "ct_send() failed\n");
     exit(1);
   }
	while(ct_results(cmd, &restype) == CS_SUCCEED) ;

	ct_dynamic(cmd, CS_PREPARE, "BUG", CS_NULLTERM, "insert tempdb..prepare_bug values(?, ?, ?, ?)", CS_NULLTERM);
	if(ct_send(cmd) != CS_SUCCEED) {
		fprintf(stderr,"ct_send failed in prepare");
		exit(1);
	}

	while(ct_results(cmd, &restype) == CS_SUCCEED) ;
/*
---- Sybase has now created a temporary stored proc
---- now - describe the parameters 
*/

	ct_dynamic(cmd, CS_DESCRIBE_INPUT, "BUG", CS_NULLTERM, NULL, CS_UNUSED);
	ct_send(cmd);
	while(ct_results(cmd, &restype) == CS_SUCCEED) {
		if(restype == CS_DESCRIBE_RESULT) {
			CS_INT num_param, outlen;
			int i;
			ct_res_info(cmd, CS_NUMDATA, &num_param, CS_UNUSED, &outlen);
			for(i = 1; i <= num_param; ++i) {
				ct_describe(cmd, i, &datafmt[i-1]);
				printf("column type: %d size: %d\n", datafmt[i-1].datatype, datafmt[i-1].maxlength);
			}
		}
	}

/*
---- Now we have CS_DATAFMT struct for each of the input params
---- to the request. 
---- so we can execute it with some parameters
*/

	execute(cmd, "BUG", &datafmt[0], "test", "Jan 1 1988", 123.4, 222.334);

/*
---- from now on it's a standard fetch all results loop...
*/

	while(ct_results(cmd, &restype) == CS_SUCCEED) {
		printf("got restype %d\n", restype);
		if(restype == CS_ROW_RESULT) {
			int numcols, rows;
			char string[20];
			int i, len, ind, ival;
			CS_DATAFMT output[2];

			ct_res_info(cmd, CS_NUMDATA, &numcols, CS_UNUSED, NULL);
			for(i = 0; i < numcols; ++i) {
				ct_describe(cmd, (i + 1), &output[i]);
				if(output[i].datatype == CS_CHAR_TYPE) {
					ct_bind(cmd, (i + 1), &output[i], &string, &len, &ind);
				} else {
					ct_bind(cmd, (i + 1), &output[i], &ival, &len, &ind);
				}
			}

			while(ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &rows) == CS_SUCCEED) {
				printf("data = %.3s - %d\n", string, ival);
			}
		}
	}
}
void execute(CS_COMMAND *cmd, char *id, CS_DATAFMT *datafmt, char *p1, char *p2,
			 double p3, double p4) 
{
CS_INT restype;

	printf("execute called with %s, %s, %f, %f\n", p1, p2, p3, p4);
	ct_dynamic(cmd, CS_EXECUTE, id, CS_NULLTERM, NULL, CS_UNUSED);

	datafmt[0].datatype = CS_CHAR_TYPE;
	ct_param(cmd, &datafmt[0], p1, CS_NULLTERM, 0);

	datafmt[1].datatype = CS_CHAR_TYPE;
	ct_param(cmd, &datafmt[1], p2, CS_NULLTERM, 0);

	datafmt[2].datatype = CS_FLOAT_TYPE;
	ct_param(cmd, &datafmt[2], &p3, sizeof(double), 0);

	datafmt[3].datatype = CS_FLOAT_TYPE;
	ct_param(cmd, &datafmt[3], &p4, sizeof(double), 0);

	ct_send(cmd);
}
