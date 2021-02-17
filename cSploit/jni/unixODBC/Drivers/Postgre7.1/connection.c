
/* Module:          connection.c
 *
 * Description:     This module contains routines related to 
 *                  connecting to and disconnecting from the Postgres DBMS.
 *
 * Classes:         ConnectionClass (Functions prefix: "CC_")
 *
 * API functions:   SQLAllocConnect, SQLConnect, SQLDisconnect, SQLFreeConnect,
 *                  SQLBrowseConnect(NI)
 *
 * Comments:        See "notice.txt" for copyright and license information.
 *
 */

#include <config.h>
#include "environ.h"
#include "connection.h"
#include "socket.h"
#include "statement.h"
#include "qresult.h"
#include "lobj.h"
#include "dlg_specific.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <odbcinst.h>
#endif

#ifdef UNIXODBC
#include <odbcinst.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif
#include "md5.h"
#endif


#define STMT_INCREMENT 16  /* how many statement holders to allocate at a time */

#define PRN_NULLCHECK

extern GLOBAL_VALUES globals;


RETCODE SQL_API SQLAllocConnect(HENV     henv,
                                HDBC FAR *phdbc)
{
EnvironmentClass *env = (EnvironmentClass *)henv;
ConnectionClass *conn;
static char* const func="SQLAllocConnect";

	mylog( "%s: entering...\n", func);

	conn = CC_Constructor();
	mylog("**** %s: henv = %u, conn = %u\n", func, henv, conn);

    if( ! conn) {
	env->errornumber = ENV_ALLOC_ERROR;
	env->errormsg = "Couldn't allocate memory for Connection object.";
	*phdbc = SQL_NULL_HDBC;
	EN_log_error(func, "", env);
	return SQL_ERROR;
    }

    if ( ! EN_add_connection(env, conn)) {
        env->errornumber = ENV_ALLOC_ERROR;
        env->errormsg = "Maximum number of connections exceeded.";
        CC_Destructor(conn);
	*phdbc = SQL_NULL_HDBC;
	EN_log_error(func, "", env);
        return SQL_ERROR;
    }

    *phdbc = (HDBC) conn;

    return SQL_SUCCESS;
}


/*      -       -       -       -       -       -       -       -       - */

RETCODE SQL_API SQLConnect(
                           HDBC      hdbc,
                           UCHAR FAR *szDSN,
                           SWORD     cbDSN,
                           UCHAR FAR *szUID,
                           SWORD     cbUID,
                           UCHAR FAR *szAuthStr,
                           SWORD     cbAuthStr)
{
ConnectionClass *conn = (ConnectionClass *) hdbc;
ConnInfo *ci;
static char* const func = "SQLConnect";

	mylog( "%s: entering...\n", func);

	if ( ! conn) {
		CC_log_error(func, "", NULL);
		return SQL_INVALID_HANDLE;
	}

	ci = &conn->connInfo;

	make_string((char*)szDSN, cbDSN, ci->dsn);

	/*	get the values for the DSN from the registry */
	getDSNinfo(ci, CONN_OVERWRITE);
	/*	initialize pg_version from connInfo.protocol	*/
	CC_initialize_pg_version(conn); 
	
	/*	override values from DSN info with UID and authStr(pwd) 
		This only occurs if the values are actually there.
	*/
	make_string((char*)szUID, cbUID, ci->username);
	make_string((char*)szAuthStr, cbAuthStr, ci->password);

	/* fill in any defaults */
	getDSNdefaults(ci);

	qlog("conn = %u, %s(DSN='%s', UID='%s', PWD='%s')\n", conn, func, ci->dsn, ci->username, ci->password);

	if ( CC_connect(conn, AUTH_REQ_OK, NULL) <= 0) {
		/*	Error messages are filled in */
		CC_log_error(func, "Error on CC_connect", conn);
		return SQL_ERROR;
	}

	mylog( "%s: returning...\n", func);

	return SQL_SUCCESS;
}

/*      -       -       -       -       -       -       -       -       - */

RETCODE SQL_API SQLBrowseConnect(
        HDBC      hdbc,
        UCHAR FAR *szConnStrIn,
        SWORD     cbConnStrIn,
        UCHAR FAR *szConnStrOut,
        SWORD     cbConnStrOutMax,
        SWORD FAR *pcbConnStrOut)
{
static char* const func="SQLBrowseConnect";

	mylog( "%s: entering...\n", func);

	return SQL_SUCCESS;
}

/*      -       -       -       -       -       -       -       -       - */

/* Drop any hstmts open on hdbc and disconnect from database */
RETCODE SQL_API SQLDisconnect(
        HDBC      hdbc)
{
ConnectionClass *conn = (ConnectionClass *) hdbc;
static char* const func = "SQLDisconnect";

	mylog( "%s: entering...\n", func);

	if ( ! conn) {
		CC_log_error(func, "", NULL);
		return SQL_INVALID_HANDLE;
	}

	qlog("conn=%u, %s\n", conn, func);

	if (conn->status == CONN_EXECUTING) {
		CC_set_error(conn, CONN_IN_USE, "A transaction is currently being executed");
		CC_log_error(func, "", conn);
		return SQL_ERROR;
	}

	mylog("%s: about to CC_cleanup\n", func);

	/*  Close the connection and free statements */
	CC_cleanup(conn);

	mylog("%s: done CC_cleanup\n", func);
	mylog("%s: returning...\n", func);

	return SQL_SUCCESS;
}


/*      -       -       -       -       -       -       -       -       - */

RETCODE SQL_API SQLFreeConnect(
        HDBC      hdbc)
{
ConnectionClass *conn = (ConnectionClass *) hdbc;
static char* const func = "SQLFreeConnect";

	mylog( "%s: entering...\n", func);
	mylog("**** in %s: hdbc=%u\n", func, hdbc);

	if ( ! conn) {
		CC_log_error(func, "", NULL);
		return SQL_INVALID_HANDLE;
	}

	/*  Remove the connection from the environment */
	if ( ! EN_remove_connection((EnvironmentClass*)conn->henv, conn)) {
		CC_set_error(conn, CONN_IN_USE, "A transaction is currently being executed");
		CC_log_error(func, "", conn);
		return SQL_ERROR;
	}

	CC_Destructor(conn);

	mylog("%s: returning...\n", func);

	return SQL_SUCCESS;
}


/*
*
*       IMPLEMENTATION CONNECTION CLASS
*
*/

ConnectionClass *CC_Constructor()
{
ConnectionClass *rv;

    rv = (ConnectionClass *)malloc(sizeof(ConnectionClass));

    if (rv != NULL) {

		rv->henv = (HENV)NULL; /* not yet associated with an environment */

        CC_clear_error(rv);

        rv->status = CONN_NOT_CONNECTED;
        rv->transact_status = CONN_IN_AUTOCOMMIT; /* autocommit by default */

		memset(&rv->connInfo, 0, sizeof(ConnInfo));

		rv->sock = SOCK_Constructor();
		if ( ! rv->sock)
			return NULL;

		rv->stmts = (StatementClass **) malloc( sizeof(StatementClass *) * STMT_INCREMENT);
		if ( ! rv->stmts)
			return NULL;
		memset(rv->stmts, 0, sizeof(StatementClass *) * STMT_INCREMENT);

		rv->num_stmts = STMT_INCREMENT;

		rv->lobj_type = PG_TYPE_LO;

		rv->ntables = 0;
		rv->col_info = NULL;

		rv->translation_option = 0;
		rv->translation_handle = NULL;
		rv->DataSourceToDriver = NULL;
		rv->DriverToDataSource = NULL;
		memset(rv->pg_version, 0, sizeof(rv->pg_version));
		rv->pg_version_number = .0;
		rv->pg_version_major = 0;
		rv->pg_version_minor = 0;


		/*	Initialize statement options to defaults */
		/*	Statements under this conn will inherit these options */

		InitializeStatementOptions(&rv->stmtOptions);


    } 
    return rv;
}


char
CC_Destructor(ConnectionClass *self)
{

	mylog("enter CC_Destructor, self=%u\n", self);

	if (self->status == CONN_EXECUTING)
		return 0;

	CC_cleanup(self);   /* cleanup socket and statements */

	mylog("after CC_Cleanup\n");

	/*  Free up statement holders */
	if (self->stmts) {
		free(self->stmts);
		self->stmts = NULL;
	}
	mylog("after free statement holders\n");

	/*	Free cached table info */
	if (self->col_info) {
		int i;
		for (i = 0; i < self->ntables; i++) {
			if (self->col_info[i]->result)		/*	Free the SQLColumns result structure */
				QR_Destructor(self->col_info[i]->result);

			free(self->col_info[i]);
		}

		CC_set_errormsg(self, NULL);
		free(self->col_info);
	}

	CC_set_errormsg(self, NULL);

	free(self);

	mylog("exit CC_Destructor\n");

	return 1;
}

/*	Return how many cursors are opened on this connection */
int
CC_cursor_count(ConnectionClass *self)
{
StatementClass *stmt;
int i, count = 0;

	mylog("CC_cursor_count: self=%u, num_stmts=%d\n", self, self->num_stmts);

	for (i = 0; i < self->num_stmts; i++) {
		stmt = self->stmts[i];
		if (stmt && stmt->result && stmt->result->cursor)
			count++;
	}

	mylog("CC_cursor_count: returning %d\n", count);

	return count;
}

void 
CC_clear_error(ConnectionClass *self)
{
	self->__error_number = 0; 
	self->__error_message = NULL; 
	self->errormsg_created = FALSE;
}

/*	Used to cancel a transaction */
/*	We are almost always in the middle of a transaction. */
char
CC_abort(ConnectionClass *self)
{
QResultClass *res;

	if ( CC_is_in_trans(self)) {
		res = NULL;

		mylog("CC_abort:  sending ABORT!\n");

		res = CC_send_query(self, "ABORT", NULL);
		CC_set_no_trans(self);

		if (res != NULL)
			QR_Destructor(res);
		else
			return FALSE;

	}

	return TRUE;
}

/* This is called by SQLDisconnect also */
char
CC_cleanup(ConnectionClass *self)
{
int i;
StatementClass *stmt;

	if (self->status == CONN_EXECUTING)
		return FALSE;

	mylog("in CC_Cleanup, self=%u\n", self);

	/* Cancel an ongoing transaction */
	/* We are always in the middle of a transaction, */
	/* even if we are in auto commit. */
	if (self->sock)
		CC_abort(self);

    /* 
     * mimic closePGconn
     */

    if ( self->sock)
    {
        SOCK_put_string( self->sock, "X" );
        SOCK_flush_output( self->sock );
    }

	mylog("after CC_abort\n");

	/*  This actually closes the connection to the dbase */
	if (self->sock) {
	    SOCK_Destructor(self->sock);
		self->sock = NULL;
	}

	mylog("after SOCK destructor\n");

	/*  Free all the stmts on this connection */
	for (i = 0; i < self->num_stmts; i++) {
        if ( self->stmts )
        {
            stmt = self->stmts[i];
            if (stmt) {

                stmt->hdbc = NULL;	/* prevent any more dbase interactions */

                SC_Destructor(stmt);

                self->stmts[i] = NULL;
            }
        }
	}

	/*	Check for translation dll */
#ifdef WIN32
	if ( self->translation_handle) {
		FreeLibrary (self->translation_handle);
		self->translation_handle = NULL;
	}
#endif

	mylog("exit CC_Cleanup\n");
	return TRUE;
}

int
CC_set_translation (ConnectionClass *self)
{

#ifdef WIN32

	if (self->translation_handle != NULL) {
		FreeLibrary (self->translation_handle);
		self->translation_handle = NULL;
	}

	if (self->connInfo.translation_dll[0] == 0)
		return TRUE;

	self->translation_option = atoi (self->connInfo.translation_option);
	self->translation_handle = LoadLibrary (self->connInfo.translation_dll);

	if (self->translation_handle == NULL) {
		CC_set_error(self, CONN_UNABLE_TO_LOAD_DLL, "Could not load the translation DLL.");
		return FALSE;
	}

	self->DataSourceToDriver
	 = (DataSourceToDriverProc) GetProcAddress (self->translation_handle,
												"SQLDataSourceToDriver");

	self->DriverToDataSource
	 = (DriverToDataSourceProc) GetProcAddress (self->translation_handle,
												"SQLDriverToDataSource");

	if (self->DataSourceToDriver == NULL || self->DriverToDataSource == NULL) {
		CC_set_error(self, CONN_UNABLE_TO_LOAD_DLL, "Could not find translation DLL functions.");
		return FALSE;
	}
#endif
	return TRUE;
}

static	int
md5_auth_send(ConnectionClass *self, const char *salt)
{
	char	*pwd1 = NULL, *pwd2 = NULL;
	ConnInfo   *ci = &(self->connInfo);
	SocketClass	*sock = self->sock;

	if (!(pwd1 = malloc(MD5_PASSWD_LEN + 1)))
		return 1;
	if (!EncryptMD5(ci->password, ci->username, strlen(ci->username), pwd1))
	{
		free(pwd1);
		return 1;
	} 
	if (!(pwd2 = malloc(MD5_PASSWD_LEN + 1)))
	{
		free(pwd1);
		return 1;
	} 
	if (!EncryptMD5(pwd1 + strlen("md5"), salt, 4, pwd2))
	{
		free(pwd2);
		free(pwd1);
		return 1;
	}
	free(pwd1);
	SOCK_put_int(sock, 4 + strlen(pwd2) + 1, 4);
	SOCK_put_n_char(sock, pwd2, strlen(pwd2) + 1);
	SOCK_flush_output(sock);
	free(pwd2);
	return 0; 
}

char 
CC_connect(ConnectionClass *self, char password_req, char *salt_para)
{
	StartupPacket	sp;
	StartupPacket6_2 sp62;
	QResultClass 	*res;
	SocketClass	 *sock;
	ConnInfo 	*ci = &(self->connInfo);
	int 		areq = -1;
	int 		beresp;
	char 		msgbuffer[ERROR_MSG_LENGTH]; 
	char 		salt[5], notice[512];
	static char* const func="CC_connect";
	char		*encoding;
	
	mylog("%s: entering...\n", func);

	if (password_req != AUTH_REQ_OK)

		sock = self->sock;		/* already connected, just authenticate */

	else {

		qlog("Global Options: Version='%s', fetch=%d, socket=%d, unknown_sizes=%d, max_varchar_size=%d, max_longvarchar_size=%d\n",
			POSTGRESDRIVERVERSION,
			globals.fetch_max, 
			globals.socket_buffersize, 
			globals.unknown_sizes, 
			globals.max_varchar_size, 
			globals.max_longvarchar_size);
		qlog("                disable_optimizer=%d, ksqo=%d, unique_index=%d, use_declarefetch=%d\n",
			globals.disable_optimizer,
			globals.ksqo,
			globals.unique_index,
			globals.use_declarefetch);
		qlog("                text_as_longvarchar=%d, unknowns_as_longvarchar=%d, bools_as_char=%d\n",
			globals.text_as_longvarchar, 
			globals.unknowns_as_longvarchar, 
			globals.bools_as_char);
		qlog("                extra_systable_prefixes='%s', conn_settings='%s'\n",
			globals.extra_systable_prefixes, 
			globals.conn_settings);

		if (self->status != CONN_NOT_CONNECTED) {
			CC_set_error(self, CONN_OPENDB_ERROR, "Already connected.");
			return 0;
		}

		if ( ci->server[0] == '\0' || ci->port[0] == '\0' || ci->database[0] == '\0') {
			CC_set_error(self, CONN_INIREAD_ERROR,"Missing server name, port, or database name in call to CC_connect.");
			return 0;
		}

		mylog("CC_connect(): DSN = '%s', server = '%s', port = '%s', database = '%s', username = '%s', password='%s'\n", ci->dsn, ci->server, ci->port, ci->database, ci->username, ci->password);

another_version_retry:

		/* If the socket was closed for some reason (like a SQLDisconnect, but no SQLFreeConnect
		 * then create a socket now.
		 */
		if ( ! self->sock) {
			self->sock = SOCK_Constructor();
			if ( ! self->sock) {
				 CC_set_error(self, CONNECTION_SERVER_NOT_REACHED, "Could not open a socket to the server");
				 return 0;
			}
		}

		sock = self->sock;

		mylog("connecting to the server socket...\n");

		SOCK_connect_to(sock, (short) atoi(ci->port), ci->server, ci->uds);
		if (SOCK_get_errcode(sock) != 0) {
			mylog("connection to the server socket failed.\n");
			CC_set_error(self, CONNECTION_SERVER_NOT_REACHED, "Could not connect to the server");
			return 0;
		}
		mylog("connection to the server socket succeeded.\n");

		if ( PROTOCOL_62(ci)) {
			sock->reverse = TRUE;		/* make put_int and get_int work for 6.2 */

			memset(&sp62, 0, sizeof(StartupPacket6_2));
			SOCK_put_int(sock, htonl(4+sizeof(StartupPacket6_2)), 4);
			sp62.authtype = htonl(NO_AUTHENTICATION);
			strncpy(sp62.database, ci->database, PATH_SIZE);
			strncpy(sp62.user, ci->username, NAMEDATALEN);
			SOCK_put_n_char(sock, (char *) &sp62, sizeof(StartupPacket6_2));
			SOCK_flush_output(sock);
		}
		else {
			memset(&sp, 0, sizeof(StartupPacket));

			mylog("sizeof startup packet = %d\n", sizeof(StartupPacket));

			/* Send length of Authentication Block */
			SOCK_put_int(sock, 4+sizeof(StartupPacket), 4); 

			if ( PROTOCOL_63(ci))
				sp.protoVersion = (ProtocolVersion) htonl(PG_PROTOCOL_63);
			else
				sp.protoVersion = (ProtocolVersion) htonl(PG_PROTOCOL_LATEST);

			strncpy(sp.database, ci->database, SM_DATABASE);
			strncpy(sp.user, ci->username, SM_USER);

			SOCK_put_n_char(sock, (char *) &sp, sizeof(StartupPacket));
			SOCK_flush_output(sock);
		}

		mylog("sent the authentication block.\n");

		if (sock->errornumber != 0) {
			mylog("couldn't send the authentication block properly.\n");
			CC_set_error(self, CONN_INVALID_AUTHENTICATION, "Sending the authentication packet failed");
			return 0;
		}
		mylog("sent the authentication block successfully.\n");
	}


	mylog("gonna do authentication\n");


	/* *************************************************** */
	/*	Now get the authentication request from backend */
	/* *************************************************** */

	if (!PROTOCOL_62(ci))
	{
		BOOL		before_64 = PG_VERSION_LT(self, 6.4),
					ReadyForQuery = FALSE;

		do
		{
			if (password_req != AUTH_REQ_OK)
				beresp = 'R';
			else
			{
				beresp = SOCK_get_char(sock);
				mylog("auth got '%c'\n", beresp);
			}

			switch (beresp)
			{
				case 'E':

					SOCK_get_string(sock, msgbuffer, ERROR_MSG_LENGTH);
					CC_set_error(self, CONN_INVALID_AUTHENTICATION, msgbuffer);
					qlog("ERROR from backend during authentication: '%s'\n", msgbuffer);
					if (strncmp(msgbuffer, "Unsupported frontend protocol", 29) == 0)
					{			/* retry older version */
						if (PROTOCOL_63(ci))
							strcpy(ci->protocol, PG62);
						else
							strcpy(ci->protocol, PG63);
						SOCK_Destructor(sock);
						self->sock = (SocketClass *) 0;
						CC_initialize_pg_version(self);
						goto another_version_retry;
					}

					return 0;
				case 'R':

					if (password_req != AUTH_REQ_OK)
					{
						mylog("in 'R' password_req=%s\n", ci->password);
						areq = password_req;
						if (salt_para)
							memcpy(salt, salt_para, sizeof(salt));
						password_req = AUTH_REQ_OK;
					}
					else
					{

						areq = SOCK_get_int(sock, 4);
						if (areq == AUTH_REQ_MD5)
							SOCK_get_n_char(sock, salt, 4);
						else if (areq == AUTH_REQ_CRYPT)
							SOCK_get_n_char(sock, salt, 2);

						mylog("areq = %d\n", areq);
					}
					switch (areq)
					{
						case AUTH_REQ_OK:
							break;

						case AUTH_REQ_KRB4:
							CC_set_error(self, CONN_AUTH_TYPE_UNSUPPORTED, "Kerberos 4 authentication not supported");
							return 0;

						case AUTH_REQ_KRB5:
							CC_set_error(self, CONN_AUTH_TYPE_UNSUPPORTED, "Kerberos 5 authentication not supported");
							return 0;

						case AUTH_REQ_PASSWORD:
							mylog("in AUTH_REQ_PASSWORD\n");

							if (ci->password[0] == '\0')
							{
								CC_set_error(self, CONNECTION_NEED_PASSWORD, "A password is required for this connection.");
								return -areq;		/* need password */
							}

							mylog("past need password\n");

							SOCK_put_int(sock, 4 + strlen(ci->password) + 1, 4);
							SOCK_put_n_char(sock, ci->password, strlen(ci->password) + 1);
							SOCK_flush_output(sock);

							mylog("past flush\n");
							break;

						case AUTH_REQ_CRYPT:
#ifdef HAVE_LIBCRYPT
                			{
                    			char *password;

                    			mylog("in AUTH_REQ_CRYPT\n");

                    			if (ci->password[0] == '\0') {
									CC_set_error(self, CONNECTION_NEED_PASSWORD, "A password is required for this connection.");
                        			return -1;	/* need password */
                    			}

                    			mylog("past need password\n");
			
                    			password = crypt( ci -> password, salt );
			
                    			SOCK_put_int(sock, 4+strlen(password)+1, 4);
                    			SOCK_put_n_char(sock, password, strlen(password) + 1);
                    			SOCK_flush_output(sock);
			
                    			mylog("past flush\n");
                    			break;
                			}
#else
							CC_set_error(self, CONN_AUTH_TYPE_UNSUPPORTED, "Password crypt authentication not supported.");
#endif
							return 0;
						case AUTH_REQ_MD5:
							mylog("in AUTH_REQ_MD5\n");
							if (ci->password[0] == '\0')
							{
								CC_set_error(self, CONNECTION_NEED_PASSWORD, "A password is required for this connection.");
								if (salt_para)
									memcpy(salt_para, salt, sizeof(salt));
								return -areq; /* need password */
							}
							if (md5_auth_send(self, salt))
							{
								CC_set_error(self, CONN_INVALID_AUTHENTICATION, "md5 hashing failed");
								return 0;
							}
							break;

						case AUTH_REQ_SCM_CREDS:
							CC_set_error(self, CONN_AUTH_TYPE_UNSUPPORTED, "Unix socket credential authentication not supported");
							return 0;

						default:
							CC_set_error(self, CONN_AUTH_TYPE_UNSUPPORTED, "Unknown authentication type");
							return 0;
					}
					break;
				case 'K':		/* Secret key (6.4 protocol) */
					self->be_pid = SOCK_get_int(sock, 4);		/* pid */
					self->be_key = SOCK_get_int(sock, 4);		/* key */

					break;
				case 'Z':		/* Backend is ready for new query (6.4) */
					ReadyForQuery = TRUE;
					break;
				case 'N':	/* Notices may come */
					while (SOCK_get_string(sock, notice, sizeof(notice) - 1)) ;
					break;
				default:
					CC_set_error(self, CONN_INVALID_AUTHENTICATION, "Unexpected protocol character during authentication");
					return 0;
			}

			/*
			 * There were no ReadyForQuery responce before 6.4.
			 */
			if (before_64 && areq == AUTH_REQ_OK)
				ReadyForQuery = TRUE;
		} while (!ReadyForQuery);
	}

	CC_set_errormsg(self, NULL);
	CC_clear_error(self);	/* clear any password error */

	/* send an empty query in order to find out whether the specified */
	/* database really exists on the server machine */
	mylog("sending an empty query...\n");

	res = CC_send_query(self, " ", NULL);
	if ( res == NULL || QR_get_status(res) != PGRES_EMPTY_QUERY) {
		mylog("got no result from the empty query.  (probably database does not exist)\n");
		CC_set_error(self, CONNECTION_NO_SUCH_DATABASE, "The database does not exist on the server\nor user authentication failed.");
		if (res != NULL)
			QR_Destructor(res);
        if( self->sock ) {
            SOCK_Destructor(self->sock);
            self->sock = NULL;
        }
	
		return 0;
	}
	if (res)
		QR_Destructor(res);

	mylog("empty query seems to be OK.\n");

	CC_set_translation (self);

	/**********************************************/
	/*******   Send any initial settings  *********/
	/**********************************************/

	/*	Since these functions allocate statements, and since the connection is not
		established yet, it would violate odbc state transition rules.  Therefore,
		these functions call the corresponding local function instead.
	*/
	CC_send_settings(self);
	CC_lookup_lo(self);		/* a hack to get the oid of our large object oid type */
	CC_lookup_pg_version(self);	/* Get PostgreSQL version for SQLGetInfo use */

	CC_set_errormsg(self, NULL);
	CC_clear_error(self);	/* clear any initial command errors */
	self->status = CONN_CONNECTED;

	mylog("%s: returning...\n", func);

	return 1;

}

char
CC_add_statement(ConnectionClass *self, StatementClass *stmt)
{
int i;

	mylog("CC_add_statement: self=%u, stmt=%u\n", self, stmt);

	for (i = 0; i < self->num_stmts; i++) {
		if ( !self->stmts[i]) {
			stmt->hdbc = self;
			self->stmts[i] = stmt;
			return TRUE;
		}
	}

	/* no more room -- allocate more memory */
	self->stmts = (StatementClass **) realloc( self->stmts, sizeof(StatementClass *) * (STMT_INCREMENT + self->num_stmts));
	if ( ! self->stmts)
		return FALSE;

	memset(&self->stmts[self->num_stmts], 0, sizeof(StatementClass *) * STMT_INCREMENT);

	stmt->hdbc = self;
	self->stmts[self->num_stmts] = stmt;

	self->num_stmts += STMT_INCREMENT;

	return TRUE;
}

char 
CC_remove_statement(ConnectionClass *self, StatementClass *stmt)
{
int i;

	for (i = 0; i < self->num_stmts; i++) {
		if (self->stmts[i] == stmt && stmt->status != STMT_EXECUTING) {
			self->stmts[i] = NULL;
			return TRUE;
		}
	}

	return FALSE;
}

/*	Create a more informative error message by concatenating the connection
	error message with its socket error message.
*/
char *
CC_create_errormsg(ConnectionClass *self)
{
	SocketClass	*sock = self->sock;
	int		pos;
	static char	msg[4096];

	mylog("enter CC_create_errormsg\n");

	msg[0] = '\0';

	if (CC_get_errormsg(self))
		strncpy(msg, CC_get_errormsg(self), sizeof(msg));

	mylog("msg = '%s'\n", msg);

	if (sock && sock->errormsg && sock->errormsg[0] != '\0') {
		pos = strlen(msg);
		sprintf(&msg[pos], ";\n%s", sock->errormsg);
	}

	mylog("exit CC_create_errormsg\n");
	return msg ? strdup(msg) : NULL;
}


void
CC_set_error(ConnectionClass *self, int number, const char *message)
{
	if (self->__error_message)
		free(self->__error_message);
	self->__error_number = number;
	self->__error_message = message ? strdup(message) : NULL;
}


void
CC_set_errormsg(ConnectionClass *self, const char *message)
{
	if (self->__error_message)
		free(self->__error_message);
	self->__error_message = message ? strdup(message) : NULL;
}

char 
CC_get_error(ConnectionClass *self, int *number, char **message)
{
	int rv;
	char *msgcrt;

	mylog("enter CC_get_error\n");

	/*	Create a very informative errormsg if it hasn't been done yet. */
	if ( ! self->errormsg_created) {
		msgcrt = CC_create_errormsg(self);
		if (self->__error_message)
			free(self->__error_message);
		self->__error_message = msgcrt;
		self->errormsg_created = TRUE;
	}

	if (CC_get_errornumber(self)) {
		*number = CC_get_errornumber(self);
		*message = CC_get_errormsg(self);
	}
	rv = (CC_get_errornumber(self) != 0);

	self->__error_number = 0;		/* clear the error */

	mylog("exit CC_get_error\n");

	return rv;
}


/*	The "result_in" is only used by QR_next_tuple() to fetch another group of rows into
	the same existing QResultClass (this occurs when the tuple cache is depleted and
	needs to be re-filled).

	The "cursor" is used by SQLExecute to associate a statement handle as the cursor name
	(i.e., C3326857) for SQL select statements.  This cursor is then used in future 
	'declare cursor C3326857 for ...' and 'fetch 100 in C3326857' statements.
*/
QResultClass *
CC_send_query(ConnectionClass *self, char *query, QueryInfo *qi)
{
QResultClass *result_in, *res = NULL;
char swallow;
int id;
SocketClass *sock = self->sock;
static char msgbuffer[MAX_MESSAGE_LEN+1];
char cmdbuffer[MAX_MESSAGE_LEN+1];	/* QR_set_command() dups this string so dont need static */


	mylog("send_query(): conn=%u, query='%s'\n", self, query);
	qlog("conn=%u, query='%s'\n", self, query);

	/* Indicate that we are sending a query to the backend */
	if(strlen(query) > MAX_MESSAGE_LEN-2) {
		CC_set_error(self, CONNECTION_MSG_TOO_LONG, "Query string is too long");
		return NULL;
	}

	if ((NULL == query) || (query[0] == '\0'))
		return NULL;

	if (SOCK_get_errcode(sock) != 0) {
		CC_set_error(self, CONNECTION_COULD_NOT_SEND, "Could not send Query to backend");
		CC_set_no_trans(self);
		return NULL;
	}

	SOCK_put_char(sock, 'Q');
	if (SOCK_get_errcode(sock) != 0) {
		CC_set_error(self, CONNECTION_COULD_NOT_SEND, "Could not send Query to backend");
		CC_set_no_trans(self);
		return NULL;
	}

	SOCK_put_string(sock, query);
	SOCK_flush_output(sock);

	if (SOCK_get_errcode(sock) != 0) {
		CC_set_error(self, CONNECTION_COULD_NOT_SEND, "Could not send Query to backend");
		CC_set_no_trans(self);
		return NULL;
	}

	mylog("send_query: done sending query\n");

	while(1) {
		/* what type of message is coming now ? */
		id = SOCK_get_char(sock);

		if ((SOCK_get_errcode(sock) != 0) || (id == EOF)) {
			CC_set_error(self, CONNECTION_NO_RESPONSE, "No response from the backend");
			if (res)
				QR_Destructor(res);

			mylog("send_query: 'id' - %s\n", CC_get_errormsg(self));
			CC_set_no_trans(self);
			return NULL;
		}

		mylog("send_query: got id = '%c'\n", id);

		switch (id) {
		case 'A' : /* Asynchronous Messages are ignored */
			(void)SOCK_get_int(sock, 4); /* id of notification */
			SOCK_get_string(sock, msgbuffer, MAX_MESSAGE_LEN);
			/* name of the relation the message comes from */
			break;
		case 'C' : /* portal query command, no tuples returned */
			/* read in the return message from the backend */
			SOCK_get_string(sock, cmdbuffer, MAX_MESSAGE_LEN);
			if (SOCK_get_errcode(sock) != 0) {
				CC_set_error(self, CONNECTION_NO_RESPONSE, "No response from backend while receiving a portal query command");
				mylog("send_query: 'C' - %s\n", CC_get_errormsg(self));
				CC_set_no_trans(self);
                if (res)
                    QR_Destructor(res);
				return NULL;
			} else {

				char clear = 0;

				mylog("send_query: ok - 'C' - %s\n", cmdbuffer);

                if (res)
                {
                    QR_Destructor(res);
                    res = NULL;
                }
                
				if (res == NULL)	/* allow for "show" style notices */
					res = QR_Constructor();

				mylog("send_query: setting cmdbuffer = '%s'\n", cmdbuffer);

				/*	Only save the first command */
				QR_set_status(res, PGRES_COMMAND_OK);
				QR_set_command(res, cmdbuffer);

				/* (Quotation from the original comments)
					since backend may produce more than one result for some commands
					we need to poll until clear
					so we send an empty query, and keep reading out of the pipe
					until an 'I' is received
				*/


				SOCK_put_string(sock, "Q ");
				SOCK_flush_output(sock);

				while( ! clear) {
					id = SOCK_get_char(sock);
					if ((SOCK_get_errcode(sock) != 0) || (id == EOF)) {
						CC_set_error(self, CONNECTION_NO_RESPONSE, "No response from the backend");
						if (res) {
							QR_Destructor(res);
						}
						mylog("send_query: id=%d error=%s \n", id, "No response from the backend");
						CC_set_no_trans(self);
						return NULL;
					}
					switch(id) {
					case 'I':
						(void) SOCK_get_char(sock);
						clear = TRUE;
						break;
					case 'Z':
						break;
					case 'C':
						SOCK_get_string(sock, cmdbuffer, ERROR_MSG_LENGTH);
						qlog("Command response: '%s'\n", cmdbuffer);
						break;
					case 'N':
						SOCK_get_string(sock, cmdbuffer, ERROR_MSG_LENGTH);
						qlog("NOTICE from backend during clear: '%s'\n", cmdbuffer);
						break;
					case 'E':
						SOCK_get_string(sock, cmdbuffer, ERROR_MSG_LENGTH);
						qlog("ERROR from backend during clear: '%s'\n", cmdbuffer);
						/* We must report this type of error as well
						   (practically for reference integrity violation
						   error reporting, from PostgreSQL 7.0).
						   (Zoltan Kovacs, 04/26/2000)
						*/
						CC_set_errormsg(self, cmdbuffer);
						if ( ! strncmp(cmdbuffer, "FATAL", 5)) {
						    CC_set_errornumber(self, CONNECTION_SERVER_REPORTED_ERROR);
						    CC_set_no_trans(self);
						    }
						else
						    CC_set_errornumber(self, CONNECTION_SERVER_REPORTED_WARNING);
						QR_set_status(res, PGRES_NONFATAL_ERROR);
						QR_set_aborted(res, TRUE);
						break;
					}
				}
				
				mylog("send_query: returning res = %u\n", res);
				return res;
			}
		case 'K':	/* Secret key (6.4 protocol) */
			(void)SOCK_get_int(sock, 4); /* pid */
			(void)SOCK_get_int(sock, 4); /* key */

			break;
		case 'Z':	/* Backend is ready for new query (6.4) */
			break;
		case 'N' : /* NOTICE: */
			SOCK_get_string(sock, cmdbuffer, ERROR_MSG_LENGTH);

            if (res)
                QR_Destructor(res);
			res = QR_Constructor();
			QR_set_status(res, PGRES_NONFATAL_ERROR);
			QR_set_notice(res, cmdbuffer);	/* will dup this string */

			mylog("~~~ NOTICE: '%s'\n", cmdbuffer);
			qlog("NOTICE from backend during send_query: '%s'\n", cmdbuffer);

			continue;		/* dont return a result -- continue reading */

		case 'I' : /* The server sends an empty query */
				/* There is a closing '\0' following the 'I', so we eat it */
			swallow = SOCK_get_char(sock);
			if ((swallow != '\0') || SOCK_get_errcode(sock) != 0) {
				CC_set_error(self, CONNECTION_BACKEND_CRAZY, "Unexpected protocol character from backend (send_query - I)");
                if (res)
                    QR_Destructor(res);
				res = QR_Constructor();
				QR_set_status(res, PGRES_FATAL_ERROR);
				return res;
			} else {
				/* We return the empty query */
                if (res)
                    QR_Destructor(res);
				res = QR_Constructor();
				QR_set_status(res, PGRES_EMPTY_QUERY);
				return res;
			}
			break;
		case 'E' : 
			SOCK_get_string(sock, msgbuffer, ERROR_MSG_LENGTH);

			/*	Remove a newline */
			if (msgbuffer[0] != '\0' && msgbuffer[strlen(msgbuffer)-1] == '\n')
				msgbuffer[strlen(msgbuffer)-1] = '\0';

			CC_set_errormsg(self, msgbuffer);

			mylog("send_query: 'E' - %s\n", msgbuffer);
			qlog("ERROR from backend during send_query: '%s'\n", msgbuffer);

			/* We should report that an error occured. Zoltan */
            if (res)
                QR_Destructor(res);
			res = QR_Constructor();

			if ( ! strncmp(msgbuffer, "FATAL", 5)) {
				CC_set_errornumber(self, CONNECTION_SERVER_REPORTED_ERROR);
				CC_set_no_trans(self);
				QR_set_status(res, PGRES_FATAL_ERROR);
			}
			else {
				CC_set_errornumber(self, CONNECTION_SERVER_REPORTED_WARNING);
				QR_set_status(res, PGRES_NONFATAL_ERROR);
			}
			QR_set_aborted(res, TRUE);

			return res; /* instead of NULL. Zoltan */

		case 'P' : /* get the Portal name */
			SOCK_get_string(sock, msgbuffer, MAX_MESSAGE_LEN);
			break;
		case 'T': /* Tuple results start here */
			result_in = qi ? qi->result_in : NULL;

			if ( result_in == NULL) {
				result_in = QR_Constructor();
				mylog("send_query: 'T' no result_in: res = %u\n", result_in);
				if ( ! result_in) {
					CC_set_error(self, CONNECTION_COULD_NOT_RECEIVE, "Could not create result info in send_query.");
                    if (res)
                        QR_Destructor(res);
					return NULL;
				}

				if (qi)
					QR_set_cache_size(result_in, qi->row_size);

				if ( ! QR_fetch_tuples(result_in, self, qi ? qi->cursor : NULL)) {
					CC_set_errornumber(self, CONNECTION_COULD_NOT_RECEIVE);
                    if (res)
                        QR_Destructor(res);
					CC_set_errormsg(self, QR_get_message(result_in));
					return NULL;
				}
			}
			else {  /* next fetch, so reuse an existing result */
				if ( ! QR_fetch_tuples(result_in, NULL, NULL)) {
					CC_set_error(self, CONNECTION_COULD_NOT_RECEIVE, QR_get_message(result_in));
                    if (res)
                        QR_Destructor(res);
					return NULL;
				}
			}

			return result_in;
		case 'D': /* Copy in command began successfully */
			if (res)
			QR_Destructor(res);
			res = QR_Constructor();
			QR_set_status(res, PGRES_COPY_IN);
			return res;
		case 'B': /* Copy out command began successfully */
			if (res)
			QR_Destructor(res);
			res = QR_Constructor();
			QR_set_status(res, PGRES_COPY_OUT);
			return res;
		default:
			CC_set_error(self, CONNECTION_BACKEND_CRAZY, "Unexpected protocol character from backend (send_query)");
			CC_set_no_trans(self);

			mylog("send_query: error - %s\n", CC_get_errormsg(self));
			if (res)
			QR_Destructor(res);
			return NULL;
		}
	}
}

int
CC_send_function(ConnectionClass *self, int fnid, void *result_buf, int *actual_result_len, int result_is_int, LO_ARG *args, int nargs)
{
char id, c, done;
SocketClass *sock = self->sock;
static char msgbuffer[MAX_MESSAGE_LEN+1];
int i;

	mylog("send_function(): conn=%u, fnid=%d, result_is_int=%d, nargs=%d\n", self, fnid, result_is_int, nargs);

	if (SOCK_get_errcode(sock) != 0) {
		CC_set_error(self, CONNECTION_COULD_NOT_SEND, "Could not send function to backend");
		CC_set_no_trans(self);
		return FALSE;
	}

	SOCK_put_string(sock, "F ");
	if (SOCK_get_errcode(sock) != 0) {
		CC_set_error(self, CONNECTION_COULD_NOT_SEND, "Could not send function to backend");
		CC_set_no_trans(self);
		return FALSE;
	}

	SOCK_put_int(sock, fnid, 4); 
	SOCK_put_int(sock, nargs, 4); 


	mylog("send_function: done sending function\n");

	for (i = 0; i < nargs; ++i) {

		mylog("  arg[%d]: len = %d, isint = %d, integer = %d, ptr = %u\n", i, args[i].len, args[i].isint, args[i].u.integer, args[i].u.ptr);

		SOCK_put_int(sock, args[i].len, 4);
		if (args[i].isint) 
			SOCK_put_int(sock, args[i].u.integer, 4);
		else
			SOCK_put_n_char(sock, (char *) args[i].u.ptr, args[i].len);


	}

	mylog("    done sending args\n");

	SOCK_flush_output(sock);
	mylog("  after flush output\n");

	done = FALSE;
	while ( ! done) {
		id = SOCK_get_char(sock);
		mylog("   got id = %c\n", id);

		switch(id) {
		case 'V':
			done = TRUE;
			break;		/* ok */

		case 'N':
			SOCK_get_string(sock, msgbuffer, ERROR_MSG_LENGTH);
			mylog("send_function(V): 'N' - %s\n", msgbuffer);
			/*	continue reading */
			break;

		case 'E':
			SOCK_get_string(sock, msgbuffer, ERROR_MSG_LENGTH);
			CC_set_errormsg(self, msgbuffer);

			mylog("send_function(V): 'E' - %s\n", msgbuffer);
			qlog("ERROR from backend during send_function: '%s'\n", msgbuffer);

			return FALSE;

		case 'Z':
			break;

		default:
			CC_set_error(self, CONNECTION_BACKEND_CRAZY, "Unexpected protocol character from backend (send_function, args)");
			CC_set_no_trans(self);

			mylog("send_function: error - %s\n", CC_get_errormsg(self));
			return FALSE;
		}
	}

	id = SOCK_get_char(sock);
	for (;;) {
		switch (id) {
		case 'G':	/* function returned properly */
			mylog("  got G!\n");

			*actual_result_len = SOCK_get_int(sock, 4);
			mylog("  actual_result_len = %d\n", *actual_result_len);

			if (result_is_int)
				*((int *) result_buf) = SOCK_get_int(sock, 4);
			else
				SOCK_get_n_char(sock, (char *) result_buf, *actual_result_len);

			mylog("  after get result\n");

			c = SOCK_get_char(sock);	/* get the last '0' */

			mylog("   after get 0\n");

			return TRUE;

		case 'E':
			SOCK_get_string(sock, msgbuffer, ERROR_MSG_LENGTH);
			CC_set_errormsg(self, msgbuffer);

			mylog("send_function(G): 'E' - %s\n", msgbuffer);
			qlog("ERROR from backend during send_function: '%s'\n", msgbuffer);

			return FALSE;

		case 'N':
			SOCK_get_string(sock, msgbuffer, ERROR_MSG_LENGTH);

			mylog("send_function(G): 'N' - %s\n", msgbuffer);
			qlog("NOTICE from backend during send_function: '%s'\n", msgbuffer);

			continue;		/* dont return a result -- continue reading */

		case '0':	/* empty result */
			return TRUE;

		default:
			CC_set_error(self, CONNECTION_BACKEND_CRAZY, "Unexpected protocol character from backend (send_function, result)");
			CC_set_no_trans(self);

			mylog("send_function: error - %s\n", CC_get_errormsg(self));
			return FALSE;
		}
	}
}


char
CC_send_settings(ConnectionClass *self)
{
  /* char ini_query[MAX_MESSAGE_LEN]; */
ConnInfo *ci = &(self->connInfo);
/* QResultClass *res; */
HSTMT hstmt;
StatementClass *stmt;
RETCODE result;
char status = TRUE;
char *cs, *ptr;
static char* const func="CC_send_settings";


	mylog("%s: entering...\n", func);

/*	This function must use the local odbc API functions since the odbc state 
	has not transitioned to "connected" yet.
*/

	result = PG_SQLAllocStmt( self, &hstmt);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		return FALSE;
	}
	stmt = (StatementClass *) hstmt;

	stmt->internal = TRUE;	/* ensure no BEGIN/COMMIT/ABORT stuff */

	/*	Set the Datestyle to the format the driver expects it to be in */
	result = PG_SQLExecDirect(hstmt, "set DateStyle to 'ISO'", SQL_NTS);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO))
		status = FALSE;

	mylog("%s: result %d, status %d from set DateStyle\n", func, result, status);

	/*	Disable genetic optimizer based on global flag */
	if (globals.disable_optimizer) {
		result = PG_SQLExecDirect(hstmt, "set geqo to 'OFF'", SQL_NTS);
		if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO))
			status = FALSE;

		mylog("%s: result %d, status %d from set geqo\n", func, result, status);
	
	}

	/*	KSQO */
	if (globals.ksqo) {
		result = PG_SQLExecDirect(hstmt, "set ksqo to 'ON'", SQL_NTS);
		if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO))
			status = FALSE;

		mylog("%s: result %d, status %d from set ksqo\n", func, result, status);
	
	}

	/*	Global settings */
	if (globals.conn_settings[0] != '\0') {
		cs = strdup(globals.conn_settings);
		ptr = strtok(cs, ";");
		while (ptr) {
			result = PG_SQLExecDirect(hstmt, ptr, SQL_NTS);
			if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO))
				status = FALSE;

			mylog("%s: result %d, status %d from '%s'\n", func, result, status, ptr);

			ptr = strtok(NULL, ";");
		}

		free(cs);
	}
	
	/*	Per Datasource settings */
	if (ci->conn_settings[0] != '\0') {
		cs = strdup(ci->conn_settings);
		ptr = strtok(cs, ";");
		while (ptr) {
			result = PG_SQLExecDirect(hstmt, ptr, SQL_NTS);
			if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO))
				status = FALSE;

			mylog("%s: result %d, status %d from '%s'\n", func, result, status, ptr);

			ptr = strtok(NULL, ";");
		}

		free(cs);
	}


	PG_SQLFreeStmt(hstmt, SQL_DROP);

	return status;
}

/*	This function is just a hack to get the oid of our Large Object oid type.
	If a real Large Object oid type is made part of Postgres, this function
	will go away and the define 'PG_TYPE_LO' will be updated.
*/
void
CC_lookup_lo(ConnectionClass *self) 
{
HSTMT hstmt;
StatementClass *stmt;
RETCODE result;
static char* const func = "CC_lookup_lo";

	mylog( "%s: entering...\n", func);

/*	This function must use the local odbc API functions since the odbc state 
	has not transitioned to "connected" yet.
*/
	result = PG_SQLAllocStmt( self, &hstmt);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		return;
	}
	stmt = (StatementClass *) hstmt;

	result = PG_SQLExecDirect(hstmt, "select oid from pg_type where typname='" PG_TYPE_LO_NAME "'", SQL_NTS);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		PG_SQLFreeStmt(hstmt, SQL_DROP);
		return;
	}

	result = PG_SQLFetch(hstmt);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		PG_SQLFreeStmt(hstmt, SQL_DROP);
		return;
	}

	result = PG_SQLGetData(hstmt, 1, SQL_C_SLONG, &self->lobj_type, sizeof(self->lobj_type), NULL);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		PG_SQLFreeStmt(hstmt, SQL_DROP);
		return;
	}

	mylog("Got the large object oid: %d\n", self->lobj_type);
	qlog("    [ Large Object oid = %d ]\n", self->lobj_type);

	result = PG_SQLFreeStmt(hstmt, SQL_DROP);
}

/*	This function initializes the version of PostgreSQL from
	connInfo.protocol that we're connected to.
	h-inoue 01-2-2001
*/
void
CC_initialize_pg_version(ConnectionClass *self) 
{
	strcpy(self->pg_version, self->connInfo.protocol); 
	if (PROTOCOL_62(&self->connInfo)) {
		self->pg_version_number = (float) 6.2;
		self->pg_version_major = 6;
		self->pg_version_minor = 2;
	} else if (PROTOCOL_63(&self->connInfo)) {
		self->pg_version_number = (float) 6.3;
		self->pg_version_major = 6;
		self->pg_version_minor = 3;
	} else {
		self->pg_version_number = (float) 6.4;
		self->pg_version_major = 6;
		self->pg_version_minor = 4;
	}
}
/*	This function gets the version of PostgreSQL that we're connected to.
    This is used to return the correct info in SQLGetInfo
	DJP - 25-1-2001
*/
void
CC_lookup_pg_version(ConnectionClass *self) 
{
HSTMT hstmt;
StatementClass *stmt;
RETCODE result;
char	szVersion[32];
int	major, minor;
static char* const func = "CC_lookup_pg_version";

	mylog( "%s: entering...\n", func);

/*	This function must use the local odbc API functions since the odbc state 
	has not transitioned to "connected" yet.
*/
	result = PG_SQLAllocStmt( self, &hstmt);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		return;
	}
	stmt = (StatementClass *) hstmt;

	/*	get the server's version if possible	*/
	result = PG_SQLExecDirect(hstmt, "select version()", SQL_NTS);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		PG_SQLFreeStmt(hstmt, SQL_DROP);
		return;
	}

	result = PG_SQLFetch(hstmt);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		PG_SQLFreeStmt(hstmt, SQL_DROP);
		return;
	}

	result = PG_SQLGetData(hstmt, 1, SQL_C_CHAR, self->pg_version, MAX_INFO_STRING, NULL);
	if((result != SQL_SUCCESS) && (result != SQL_SUCCESS_WITH_INFO)) {
		PG_SQLFreeStmt(hstmt, SQL_DROP);
		return;
	}

	/* Extract the Major and Minor numbers from the string. */
	/* This assumes the string starts 'Postgresql X.X' */
	strcpy(szVersion, "0.0");
	if (sscanf(self->pg_version, "%*s %d.%d", &major, &minor) >= 2) {
		sprintf(szVersion, "%d.%d", major, minor);
		self->pg_version_major = major;
		self->pg_version_minor = minor;
	}
	self->pg_version_number = (float) atof(szVersion);

	mylog("Got the PostgreSQL version string: '%s'\n", self->pg_version);
	mylog("Extracted PostgreSQL version number: '%1.1f'\n", self->pg_version_number);
	qlog("    [ PostgreSQL version string = '%s' ]\n", self->pg_version);
	qlog("    [ PostgreSQL version number = '%1.1f' ]\n", self->pg_version_number);

	result = PG_SQLFreeStmt(hstmt, SQL_DROP);
}

void
CC_log_error(char *func, char *desc, ConnectionClass *self)
{
#ifdef PRN_NULLCHECK
#define nullcheck(a) (a ? a : "(NULL)")
#endif

	if (self) {
		qlog("CONN ERROR: func=%s, desc='%s', errnum=%d, errmsg='%s'\n", func, desc, self->__error_number, nullcheck (self->__error_message));
		mylog("CONN ERROR: func=%s, desc='%s', errnum=%d, errmsg='%s'\n", func, desc, self->__error_number, nullcheck (self->__error_message));
		qlog("            ------------------------------------------------------------\n");
		qlog("            henv=%u, conn=%u, status=%u, num_stmts=%d\n", self->henv, self, self->status, self->num_stmts);
		qlog("            sock=%u, stmts=%u, lobj_type=%d\n", self->sock, self->stmts, self->lobj_type);

		qlog("            ---------------- Socket Info -------------------------------\n");
		if (self->sock) {
		SocketClass *sock = self->sock;
		qlog("            socket=%d, reverse=%d, errornumber=%d, errormsg='%s'\n", sock->socket, sock->reverse, sock->errornumber, nullcheck(sock->errormsg));
		qlog("            buffer_in=%u, buffer_out=%u\n", sock->buffer_in, sock->buffer_out);
		qlog("            buffer_filled_in=%d, buffer_filled_out=%d, buffer_read_in=%d\n", sock->buffer_filled_in, sock->buffer_filled_out, sock->buffer_read_in);
		}
	}
	else
		qlog("INVALID CONNECTION HANDLE ERROR: func=%s, desc='%s'\n", func, desc);
#undef PRN_NULLCHECK
}

