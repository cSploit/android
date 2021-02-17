
#ifndef COMMON_h
#define COMMON_h

static char rcsid_common_h[] = "$Id: common.h,v 1.11 2008-07-06 16:44:24 jklowden Exp $";
static void *no_unused_common_h_warn[] = { rcsid_common_h, no_unused_common_h_warn };

extern char SERVER[512];
extern char DATABASE[512];
extern char USER[512];
extern char PASSWORD[512];

typedef struct 
{
	int initialized;
	char SERVER[512];
	char DATABASE[512];
	char USER[512];
	char PASSWORD[512];
	char fverbose;
	int maxlength; 
} COMMON_PWD;
extern COMMON_PWD common_pwd;

CS_RETCODE read_login_info(void);

extern int cslibmsg_cb_invoked;
extern int clientmsg_cb_invoked;
extern int servermsg_cb_invoked;

CS_RETCODE try_ctlogin(CS_CONTEXT ** ctx, CS_CONNECTION ** conn, CS_COMMAND ** cmd, int verbose);
CS_RETCODE try_ctlogin_with_options(int argc, char **argv, CS_CONTEXT ** ctx, CS_CONNECTION ** conn, CS_COMMAND ** cmd, 
				    int verbose);

CS_RETCODE try_ctlogout(CS_CONTEXT * ctx, CS_CONNECTION * conn, CS_COMMAND * cmd, int verbose);

CS_RETCODE run_command(CS_COMMAND * cmd, const char *sql);
CS_RETCODE cslibmsg_cb(CS_CONTEXT * connection, CS_CLIENTMSG * errmsg);
CS_RETCODE clientmsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_CLIENTMSG * errmsg);
CS_RETCODE servermsg_cb(CS_CONTEXT * context, CS_CONNECTION * connection, CS_SERVERMSG * srvmsg);

#endif
